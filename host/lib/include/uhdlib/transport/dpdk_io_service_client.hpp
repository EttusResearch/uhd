//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once

#include <uhdlib/transport/dpdk/common.hpp>
#include <uhdlib/transport/dpdk/service_queue.hpp>
#include <uhdlib/transport/dpdk/udp.hpp>
#include <uhdlib/transport/dpdk_io_service.hpp>
#include <uhdlib/transport/udp_dpdk_link.hpp>
#include <rte_ring.h>
#include <chrono>

namespace uhd { namespace transport {

struct dpdk_flow_data
{
    //! The UDP DPDK link
    udp_dpdk_link* link;
    //! Is it a recv_link_if? Or a send_link_if?
    bool is_recv;
};

/*! DPDK I/O interface for service requests
 *
 * This is used to pass around information about the I/O clients. It is what is
 * passed into the data portion of a request, for connect and disconnect
 * requests.
 *
 */
struct dpdk_io_if
{
    dpdk_io_if(bool is_recv,
        udp_dpdk_link* link,
        dpdk_io_service::sptr io_srv,
        recv_callback_t recv_cb)
        : is_recv(is_recv), link(link), io_srv(io_srv), recv_cb(recv_cb)
    {
    }

    bool is_recv;
    udp_dpdk_link* link;
    dpdk_io_service::sptr io_srv;
    recv_callback_t recv_cb;
    void* io_client;
    //! Embedded list node
    dpdk_io_if* next = NULL;
    dpdk_io_if* prev = NULL;
};

// This must be tied to a link for frame_buffs
// so need map of dpdk_send_io to udp_dpdk_link
// Have queue pair: buffs to send + free buffs
// There used to be a retry queue: Still needed? (Maybe 1 per port?)
class dpdk_send_io : public virtual send_io_if
{
public:
    using sptr = std::shared_ptr<dpdk_send_io>;

    dpdk_send_io(dpdk_io_service::sptr io_srv,
        udp_dpdk_link* link,
        size_t num_send_frames,
        send_callback_t send_cb,
        size_t num_recv_frames,
        recv_callback_t recv_cb,
        fc_callback_t fc_cb)
        : _dpdk_io_if(false, link, io_srv, recv_cb)
        , _servq(io_srv->_servq)
        , _send_cb(send_cb)
        , _fc_cb(fc_cb)
    {
        // Get reference to DPDK context, since this owns some DPDK memory
        _ctx             = dpdk::dpdk_ctx::get();
        _num_send_frames = num_send_frames;
        _num_recv_frames = num_recv_frames;

        // Create the free buffer and send queues
        // Must be power of 2, and add one since usable ring is size-1
        size_t queue_size        = (size_t)exp2(ceil(log2(num_send_frames + 1)));
        dpdk::port_id_t nic_port = link->get_port()->get_port_id();
        uint16_t id              = io_srv->_get_unique_client_id();
        char name[16];
        snprintf(name, sizeof(name), "tx%hu-%hu", nic_port, id);
        _buffer_queue = rte_ring_create(
            name, queue_size, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
        snprintf(name, sizeof(name), "~tx%hu-%hu", nic_port, id);
        _send_queue = rte_ring_create(
            name, queue_size, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
        UHD_LOG_TRACE("DPDK::SEND_IO", "dpdk_send_io() " << _buffer_queue->name);

        // Create the wait_request object that gets passed around
        _waiter = dpdk::wait_req_alloc(dpdk::wait_type::WAIT_TX_BUF, (void*)&_dpdk_io_if);
        _waiter->complete = true;
    }

    ~dpdk_send_io()
    {
        UHD_LOG_TRACE("DPDK::SEND_IO", "~dpdk_send_io() " << _buffer_queue->name);
        // Deregister with I/O service
        auto xport_req = dpdk::wait_req_alloc(
            dpdk::wait_type::WAIT_XPORT_DISCONNECT, (void*)&_dpdk_io_if);
        _servq.submit(xport_req, std::chrono::microseconds(-1));

        // Clean up
        wait_req_put(xport_req);
        rte_ring_free(_send_queue);
        rte_ring_free(_buffer_queue);
        wait_req_put(_waiter);
    }

    bool wait_for_dest_ready(size_t /*num_bytes*/, int32_t /*timeout_ms*/)
    {
        // For this I/O service, the destination is the queue to the offload
        // thread. The queue is always able to accomodate new packets since it
        // is sized to fit all the frames reserved from the link.
        return true;
    }

    frame_buff::uptr get_send_buff(int32_t timeout_ms)
    {
        frame_buff* buff_ptr;
        if (rte_ring_dequeue(_buffer_queue, (void**)&buff_ptr)) {
            if (!timeout_ms) {
                return frame_buff::uptr();
            }
            // Nothing in the queue. Try waiting if there is a timeout.
            auto timeout_point =
                std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
            std::unique_lock<std::mutex> lock(_waiter->mutex);
            wait_req_get(_waiter);
            _waiter->complete = false;
            auto is_complete  = [this] { return _waiter->complete; };
            if (timeout_ms < 0) {
                _waiter->cond.wait(lock, is_complete);
            } else {
                auto status = _waiter->cond.wait_until(lock, timeout_point, is_complete);
                if (!status) {
                    return frame_buff::uptr();
                }
            }
            // Occasionally the conditional variable wait method returns but the
            // first dequeue operation fails, even though we push onto it before
            // setting complete to true. Retrying successfully dequeues a value
            // in those cases.
            while (rte_ring_dequeue(_buffer_queue, (void**)&buff_ptr)) {
            }
        }
        return frame_buff::uptr(buff_ptr);
    }

    void release_send_buff(frame_buff::uptr buff)
    {
        auto buff_ptr = (dpdk::dpdk_frame_buff*)buff.release();
        assert(buff_ptr);
        int status = rte_ring_enqueue(_send_queue, buff_ptr);
        if (status != 0) {
            assert(false);
        }
        // TODO: Should we retry if it failed?
    }

private:
    friend class dpdk_io_service;

    dpdk_io_if _dpdk_io_if;
    size_t _num_frames_in_use = 0;

    dpdk::service_queue& _servq;
    dpdk::dpdk_ctx::sptr _ctx;
    struct rte_ring* _buffer_queue;
    struct rte_ring* _send_queue;
    dpdk::wait_req* _waiter;
    send_callback_t _send_cb;
    fc_callback_t _fc_cb;
};

class dpdk_recv_io : public virtual recv_io_if
{
public:
    using sptr = std::shared_ptr<dpdk_recv_io>;

    dpdk_recv_io(dpdk_io_service::sptr io_srv,
        udp_dpdk_link* link,
        size_t num_recv_frames,
        recv_callback_t recv_cb,
        size_t num_send_frames,
        fc_callback_t fc_cb)
        : _dpdk_io_if(true, link, io_srv, recv_cb)
        , _servq(io_srv->_servq)
        , _fc_cb(fc_cb) // Call on release
    {
        // Get reference to DPDK context, since this owns some DPDK memory
        _ctx             = dpdk::dpdk_ctx::get();
        _num_send_frames = num_send_frames;
        _num_recv_frames = num_recv_frames;
        // Create the recv and release queues
        // Must be power of 2, and add one since usable ring is size-1
        size_t queue_size        = (size_t)exp2(ceil(log2(num_recv_frames + 1)));
        dpdk::port_id_t nic_port = link->get_port()->get_port_id();
        uint16_t id              = io_srv->_get_unique_client_id();
        UHD_LOG_DEBUG(
            "DPDK::IO_SERVICE", "Creating recv client with queue size of " << queue_size);
        char name[16];
        snprintf(name, sizeof(name), "rx%hu-%hu", nic_port, id);
        _recv_queue = rte_ring_create(
            name, queue_size, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
        snprintf(name, sizeof(name), "~rx%hu-%hu", nic_port, id);
        _release_queue = rte_ring_create(
            name, queue_size, rte_socket_id(), RING_F_SP_ENQ | RING_F_SC_DEQ);
        UHD_LOG_TRACE("DPDK::RECV_IO", "dpdk_recv_io() " << _recv_queue->name);
        // Create the wait_request object that gets passed around
        _waiter = dpdk::wait_req_alloc(dpdk::wait_type::WAIT_RX, (void*)&_dpdk_io_if);
        _waiter->complete = true;
    }

    ~dpdk_recv_io()
    {
        // Deregister with I/O service
        UHD_LOG_TRACE("DPDK::RECV_IO", "~dpdk_recv_io() " << _recv_queue->name);
        auto xport_req = dpdk::wait_req_alloc(
            dpdk::wait_type::WAIT_XPORT_DISCONNECT, (void*)&_dpdk_io_if);
        _servq.submit(xport_req, std::chrono::microseconds(-1));

        // Clean up
        wait_req_put(xport_req);
        rte_ring_free(_recv_queue);
        rte_ring_free(_release_queue);
        wait_req_put(_waiter);
    }

    frame_buff::uptr get_recv_buff(int32_t timeout_ms)
    {
        frame_buff* buff_ptr;
        if (rte_ring_dequeue(_recv_queue, (void**)&buff_ptr)) {
            if (!timeout_ms) {
                return frame_buff::uptr();
            }
            // Nothing in the queue. Try waiting if there is a timeout.
            auto timeout_point =
                std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout_ms);
            std::unique_lock<std::mutex> lock(_waiter->mutex);
            wait_req_get(_waiter);
            _waiter->complete = false;
            auto is_complete  = [this] { return _waiter->complete; };
            if (timeout_ms < 0) {
                _waiter->cond.wait(lock, is_complete);
            } else {
                auto status = _waiter->cond.wait_until(lock, timeout_point, is_complete);
                if (!status) {
                    return frame_buff::uptr();
                }
            }
            // Occasionally the conditional variable wait method returns but the
            // first dequeue operation fails, even though we push onto it before
            // setting complete to true. Retrying successfully dequeues a value
            // in those cases.
            while (rte_ring_dequeue(_recv_queue, (void**)&buff_ptr)) {
            }
        }
        return frame_buff::uptr(buff_ptr);
    }

    void release_recv_buff(frame_buff::uptr buff)
    {
        frame_buff* buff_ptr = buff.release();
        int status           = rte_ring_enqueue(_release_queue, buff_ptr);
        if (status != 0) {
            assert(false);
        }
    }

private:
    friend class dpdk_io_service;

    dpdk_io_if _dpdk_io_if;
    size_t _num_frames_in_use = 0;

    dpdk::service_queue& _servq;
    dpdk::dpdk_ctx::sptr _ctx;
    struct rte_ring* _recv_queue;
    struct rte_ring* _release_queue;
    dpdk::wait_req* _waiter;
    fc_callback_t _fc_cb;
};


}} // namespace uhd::transport

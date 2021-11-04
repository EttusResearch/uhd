//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/transport/frame_buff.hpp>
#include <cassert>
#include <chrono>
#include <thread>

namespace uhd { namespace transport {

namespace detail {

// Implementation of get_send_buff used below by send and recv clients
template <typename pop_func_t>
static frame_buff::uptr client_get_buff(pop_func_t pop, const int32_t timeout_ms)
{
    using namespace std::chrono;

    if (timeout_ms == 0) {
        return frame_buff::uptr(pop());
    }

    const auto end_time = steady_clock::now() + milliseconds(timeout_ms);

    bool last_check = false;

    while (true) {
        if (frame_buff* buff = pop()) {
            return frame_buff::uptr(buff);
        }

        if (timeout_ms > 0 && steady_clock::now() > end_time) {
            if (last_check) {
                return nullptr;
            } else {
                last_check = true;
            }
        }
        std::this_thread::yield();
    }
}

} // namespace detail

/*!
 * Recv I/O client for offload I/O service
 */
template <typename io_service_t, bool polling>
class offload_recv_io : public recv_io_if
{
public:
    offload_recv_io(typename io_service_t::sptr io_srv,
        size_t num_recv_frames,
        size_t num_send_frames,
        typename io_service_t::client_port_t::sptr& port)
        : _io_srv(io_srv), _port(port)
    {
        _num_recv_frames = num_recv_frames;
        _num_send_frames = num_send_frames;
    }

    ~offload_recv_io()
    {
        assert(_num_frames_in_use == 0);

        if (_io_srv) {
            _port->client_disconnect();
        }
    }

    frame_buff::uptr get_recv_buff(int32_t timeout_ms)
    {
        if (polling) {
            return detail::client_get_buff(
                [this]() {
                    frame_buff* buff = _port->client_pop();
                    _num_frames_in_use += buff ? 1 : 0;
                    return buff;
                },
                timeout_ms);
        } else {
            frame_buff* buff = _port->client_pop(timeout_ms);
            _num_frames_in_use += buff ? 1 : 0;
            return frame_buff::uptr(buff);
        }
    }

    void release_recv_buff(frame_buff::uptr buff)
    {
        assert(buff);
        _port->client_push(buff.release());
        _num_frames_in_use--;
    }

private:
    offload_recv_io()                       = delete;
    offload_recv_io(const offload_recv_io&) = delete;

    typename io_service_t::sptr _io_srv;
    typename io_service_t::client_port_t::sptr _port;
    size_t _num_frames_in_use = 0;
};

/*!
 * Send I/O client for offload I/O service
 */
template <typename io_service_t, bool polling>
class offload_send_io : public send_io_if
{
public:
    offload_send_io(typename io_service_t::sptr io_srv,
        size_t num_recv_frames,
        size_t num_send_frames,
        typename io_service_t::client_port_t::sptr& port)
        : _io_srv(io_srv), _port(port)
    {
        _num_recv_frames = num_recv_frames;
        _num_send_frames = num_send_frames;
    }

    ~offload_send_io()
    {
        assert(_num_frames_in_use == 0);

        if (_io_srv) {
            _port->client_disconnect();
        }
    }

    bool wait_for_dest_ready(size_t /*num_bytes*/, int32_t /*timeout_ms*/)
    {
        // For offload_io_service, the destination is the queue to the offload
        // thread. The queue is always able to accomodate new packets since it
        // is sized to fit all the frames reserved from the link.
        return true;
    }

    frame_buff::uptr get_send_buff(int32_t timeout_ms)
    {
        if (polling) {
            return detail::client_get_buff(
                [this]() {
                    frame_buff* buff = _port->client_pop();
                    _num_frames_in_use += buff ? 1 : 0;
                    return buff;
                },
                timeout_ms);
        } else {
            frame_buff* buff = _port->client_pop(timeout_ms);
            _num_frames_in_use += buff ? 1 : 0;
            return frame_buff::uptr(buff);
        }
    }

    void release_send_buff(frame_buff::uptr buff)
    {
        assert(buff);
        _port->client_push(buff.release());
        _num_frames_in_use--;
    }

private:
    offload_send_io()                       = delete;
    offload_send_io(const offload_send_io&) = delete;

    typename io_service_t::sptr _io_srv;
    typename io_service_t::client_port_t::sptr _port;
    size_t _num_frames_in_use = 0;
};

}} // namespace uhd::transport

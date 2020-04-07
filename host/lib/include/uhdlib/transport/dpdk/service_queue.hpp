//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#pragma once

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <condition_variable>
#include <rte_malloc.h>
#include <rte_ring.h>
#include <chrono>
#include <memory>
#include <mutex>

namespace uhd { namespace transport { namespace dpdk {

enum wait_type {
    //! Wake immediately
    WAIT_SIMPLE,
    //! Wake upon receiving an RX packet
    WAIT_RX,
    //! Wake when a TX buffer becomes available
    WAIT_TX_BUF,
    //! Wake once the new flow/socket is created
    WAIT_FLOW_OPEN,
    //! Wake once the flow/socket is destroyed
    WAIT_FLOW_CLOSE,
    //! Wake once the new transport is connected
    WAIT_XPORT_CONNECT,
    //! Wake once the transport is disconnected
    WAIT_XPORT_DISCONNECT,
    //! Wake when MAC address found for IP address
    WAIT_ARP,
    //! Wake once the I/O worker terminates
    WAIT_LCORE_TERM,
    //! Number of possible reasons for waiting
    WAIT_TYPE_COUNT
};

/*!
 * Aggregate representing a request for the DPDK I/O service and a corresponding
 * entry in the wait queue
 *
 * This class is managed with explicit reference counting.
 */
struct wait_req
{
    //! The reason we're waiting (and service request associated with it)
    enum wait_type reason;
    //! The data associated with the service request (unmanaged by wait_req)
    void* data;
    //! A condition variable for waiting on the request
    std::condition_variable cond;
    //! The mutex associated with the condition variable
    std::mutex mutex;
    //! Whether the request was completed
    bool complete;
    //! The status or error code associated with the request
    int retval;
    //! An atomic reference counter for managing this request object's memory
    rte_atomic32_t refcnt;
};

/*!
 * Create a wait request
 *
 * \param t The reason for the wait or the service to be performed
 * \param priv_data The data associated with this wait type
 */
inline wait_req* wait_req_alloc(wait_type t, void* priv_data)
{
    wait_req* req = (wait_req*)rte_zmalloc(NULL, sizeof(*req), 0);
    if (!req) {
        return NULL;
    }
    req         = new (req) wait_req();
    req->reason = t;
    req->data   = priv_data;
    rte_atomic32_set(&req->refcnt, 1);
    return req;
}

/*!
 * Release a reference on the wait request.
 * Frees resources if the refcnt drops to 0.
 *
 * \param req The wait request upon which to decrement refcnt
 */
inline void wait_req_put(wait_req* req)
{
    if (rte_atomic32_dec_and_test(&req->refcnt)) {
        rte_free(req);
    }
}

/*!
 * Get a reference to the wait request.
 * Increments the refcnt of the wait request
 *
 * \param req The wait request upon which to increment refcnt
 */
inline void wait_req_get(wait_req* req)
{
    rte_atomic32_inc(&req->refcnt);
}


/*!
 * A service queue to funnel requests from requesters to a single servicer.
 *
 * The DPDK I/O service uses this queue to process threads waiting for various
 * reasons, such as creating/destroying a packet flow, receiving a packet, or
 * getting a buffer for TX.
 */
class service_queue
{
public:
    /*!
     * Create a service queue
     * \param depth Must be a power of 2. Actual size is less.
     * \param lcore_id The DPDK lcore_id associated with this service queue
     */
    service_queue(size_t depth, unsigned int lcore_id)
    {
        std::string name = std::string("servq") + std::to_string(lcore_id);
        _waiter_ring     = rte_ring_create(
            name.c_str(), depth, rte_lcore_to_socket_id(lcore_id), RING_F_SC_DEQ);
        if (!_waiter_ring) {
            throw uhd::runtime_error("DPDK: Failed to create the service queue");
        }
    }

    ~service_queue()
    {
        rte_ring_free(_waiter_ring);
    }

    /*!
     * Submit a wait/service request to the service queue
     * Negative timeouts indicate to block indefinitely
     *
     * This is the only function intended for the requester
     *
     * \param req The wait request to wait on
     * \param timeout How long to wait, with negative values indicating indefinitely
     * \return 0 for no timeout, -ETIMEDOUT if there was a timeout
     */
    int submit(wait_req* req, std::chrono::microseconds timeout)
    {
        auto timeout_point = std::chrono::steady_clock::now() + timeout;
        std::unique_lock<std::mutex> lock(req->mutex);
        /* Get a reference here, to be consumed by other thread (handshake) */
        wait_req_get(req);
        req->complete = false;
        if (rte_ring_enqueue(_waiter_ring, req)) {
            wait_req_put(req);
            return -ENOBUFS;
        }
        auto is_complete = [req] { return req->complete; };
        if (timeout < std::chrono::microseconds(0)) {
            req->cond.wait(lock, is_complete);
        } else {
            auto status = req->cond.wait_until(lock, timeout_point, is_complete);
            if (!status) {
                return -ETIMEDOUT;
            }
        }
        return 0;
    }

    /*!
     * Pop off the next request from the queue
     *
     * This should only be called by the servicer
     * \return A pointer to the next wait request, else NULL
     */
    wait_req* pop()
    {
        wait_req* req;
        if (rte_ring_dequeue(_waiter_ring, (void**)&req)) {
            return NULL;
        }
        return req;
    }

    /*!
     * Attempt to requeue a request to the service queue
     *
     * This should only be called by the servicer
     * \param A pointer to the wait request to requeue
     * \return 0 for success, -ENOBUFS if there was no space in the queue
     */
    int requeue(wait_req* req)
    {
        if (rte_ring_enqueue(_waiter_ring, req)) {
            return -ENOBUFS;
        }
        return 0;
    }

    /*!
     * Attempt to wake the waiter for this request
     * If successful, decrement the reference count on the wait request.
     * If the waiter could not be woken up, attempt to requeue the request and
     * change its type to WAIT_SIMPLE.
     *
     * This should only be called by the servicer.
     *
     * \param req The wait request with the waiter to wake
     * \return 0 if successful, -EAGAIN if the waiter was requeued, -ENOBUFS
     *         if the waiter could not be requeued
     */
    int complete(wait_req* req)
    {
        // Grabbing the mutex only to avoid this sequence:
        // A: Enqueue wait request
        // B: Pull wait request and satisfy
        // B: notify() on the condition variable
        // A: wait_until() on the condition variable, possibly indefinitely...
        bool stat = req->mutex.try_lock();
        if (!stat) {
            if (rte_ring_enqueue(_waiter_ring, req)) {
                UHD_LOG_WARNING("DPDK", "Could not lock wait_req mutex or requeue");
                return -ENOBUFS;
            } else {
                req->reason = WAIT_SIMPLE;
                return -EAGAIN;
            }
        }
        req->complete = true;
        req->cond.notify_one();
        req->mutex.unlock();
        wait_req_put(req);
        return stat;
    }

    /*!
     * Get the size of the service queue
     * \return size of the service queue
     */
    inline size_t get_size()
    {
        return rte_ring_get_size(_waiter_ring);
    }

    /*!
     * Get the capacity of the service queue
     * \return capacity of the service queue
     */
    inline size_t get_capacity()
    {
        return rte_ring_get_capacity(_waiter_ring);
    }

private:
    //! Multi-producer, single-consumer ring for requests
    struct rte_ring* _waiter_ring;
};

}}} // namespace uhd::transport::dpdk

//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/zero_copy_recv_offload.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/buffer_pool.hpp>

#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/thread.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>

using namespace uhd;
using namespace uhd::transport;

typedef bounded_buffer<managed_recv_buffer::sptr> bounded_buffer_t;

/***********************************************************************
 * Zero copy offload transport:
 * An intermediate transport that utilizes threading to free
 * the main thread from any receive work.
 **********************************************************************/
class zero_copy_recv_offload_impl : public zero_copy_recv_offload {
public:
    typedef boost::shared_ptr<zero_copy_recv_offload_impl> sptr;

    zero_copy_recv_offload_impl(zero_copy_if::sptr transport,
                          const double timeout) :
        _transport(transport), _timeout(timeout),
        _inbox(transport->get_num_recv_frames()),
        _recv_done(false)
    {
        UHD_LOGGER_TRACE("XPORT") << "Created threaded transport" ;

        // Create the receive and send threads to offload
        // the system calls onto other threads
        _recv_thread = boost::thread(
            boost::bind(&zero_copy_recv_offload_impl::enqueue_recv, this)
        );
        set_thread_name(&_recv_thread, "zero_copy_recv");
    }

    // Receive thread flags
    void set_recv_done()
    {
        boost::lock_guard<boost::mutex> guard(_recv_mutex);
        _recv_done = true;
    }

    bool is_recv_done()
    {
        boost::lock_guard<boost::mutex> guard(_recv_mutex);
        return _recv_done;
    }

    ~zero_copy_recv_offload_impl()
    {
        // Signal the threads we're finished
        set_recv_done();

        // Wait for them to join
        UHD_SAFE_CALL(
            _recv_thread.join();
        )
    }

    // The receive thread function is responsible for
    // pulling pointers to managed receiver buffers quickly
    void enqueue_recv()
    {
        while (not is_recv_done()) {
            managed_recv_buffer::sptr buff = _transport->get_recv_buff(_timeout);
            if (not buff) continue;
            _inbox.push_with_timed_wait(buff, _timeout);
        }
    }

    /*******************************************************************
     * Receive implementation:
     * Pop the receive buffer pointer from the underlying transport
     ******************************************************************/
    managed_recv_buffer::sptr get_recv_buff(double timeout)
    {
        managed_recv_buffer::sptr ptr;
        _inbox.pop_with_timed_wait(ptr, timeout);
        return ptr;
    }

    size_t get_num_recv_frames() const
    {
        return _transport->get_num_recv_frames();
    }

    size_t get_recv_frame_size() const
    {
        return _transport->get_recv_frame_size();
    }

    /*******************************************************************
     * Send implementation:
     * Pass the send buffer pointer from the underlying transport
     ******************************************************************/
    managed_send_buffer::sptr get_send_buff(double timeout)
    {
        return _transport->get_send_buff(timeout);
    }

    size_t get_num_send_frames() const
    {
        return _transport->get_num_send_frames();
    }

    size_t get_send_frame_size() const
    {
        return _transport->get_send_frame_size();
    }

private:
    // The linked transport
    zero_copy_if::sptr _transport;

    const double _timeout;

    // Shared buffers
    bounded_buffer_t _inbox;

    // Threading
    bool _recv_done;
    boost::thread _recv_thread;
    boost::mutex  _recv_mutex;
};

zero_copy_recv_offload::sptr zero_copy_recv_offload::make(
        zero_copy_if::sptr transport,
        const double timeout)
{
    zero_copy_recv_offload_impl::sptr zero_copy_recv_offload(
        new zero_copy_recv_offload_impl(transport, timeout)
    );

    return zero_copy_recv_offload;
}

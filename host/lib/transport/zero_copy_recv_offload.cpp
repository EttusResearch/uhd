//
// Copyright 2016 Ettus Research
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <uhd/transport/zero_copy_recv_offload.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
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
        UHD_LOG << "Created threaded transport" << std::endl;

        // Create the receive and send threads to offload
        // the system calls onto other threads
        _recv_thread = boost::thread(
            boost::bind(&zero_copy_recv_offload_impl::enqueue_recv, this)
        );
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

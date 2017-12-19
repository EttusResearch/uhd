//
// Copyright 2010-2014 Ettus Research LLC
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

#include "udp_common.hpp"
#include <uhd/transport/tcp_zero_copy.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/atomic.hpp>
#include <boost/format.hpp>
#include <boost/make_shared.hpp>
#include <boost/thread/thread.hpp> //sleep
#include <vector>

using namespace uhd;
using namespace uhd::transport;
namespace asio = boost::asio;

static const size_t DEFAULT_NUM_FRAMES = 32;
static const size_t DEFAULT_FRAME_SIZE = 2048;

/***********************************************************************
 * Reusable managed receiver buffer:
 *  - get_new performs the recv operation
 **********************************************************************/
class tcp_zero_copy_asio_mrb : public managed_recv_buffer{
public:
    tcp_zero_copy_asio_mrb(void *mem, int sock_fd, const size_t frame_size):
        _mem(mem), _sock_fd(sock_fd), _frame_size(frame_size) { /*NOP*/ }

    void release(void){
        _claimer.release();
    }

    UHD_INLINE sptr get_new(const double timeout, size_t &index){
        if (not _claimer.claim_with_wait(timeout)) return sptr();

        #ifdef MSG_DONTWAIT //try a non-blocking recv() if supported
        _len = ::recv(_sock_fd, (char *)_mem, _frame_size, MSG_DONTWAIT);
        if (_len > 0){
            index++; //advances the caller's buffer
            return make(this, _mem, size_t(_len));
        }
        #endif

        if (wait_for_recv_ready(_sock_fd, timeout)){
            _len = ::recv(_sock_fd, (char *)_mem, _frame_size, 0);
            index++; //advances the caller's buffer
            return make(this, _mem, size_t(_len));
        }

        _claimer.release(); //undo claim
        return sptr(); //null for timeout
    }

private:
    void *_mem;
    int _sock_fd;
    size_t _frame_size;
    ssize_t _len;
    simple_claimer _claimer;
};

/***********************************************************************
 * Reusable managed send buffer:
 *  - commit performs the send operation
 **********************************************************************/
class tcp_zero_copy_asio_msb : public managed_send_buffer{
public:
    tcp_zero_copy_asio_msb(void *mem, int sock_fd, const size_t frame_size):
        _mem(mem), _sock_fd(sock_fd), _frame_size(frame_size) { /*NOP*/ }

    void release(void){
        //Retry logic because send may fail with ENOBUFS.
        //This is known to occur at least on some OSX systems.
        //But it should be safe to always check for the error.
        while (true)
        {
            this->commit(_frame_size); //always full size frames to avoid pkt coalescing
            const ssize_t ret = ::send(_sock_fd, (const char *)_mem, size(), 0);
            if (ret == ssize_t(size())) break;
            if (ret == -1 and errno == ENOBUFS)
            {
                boost::this_thread::sleep(boost::posix_time::microseconds(1));
                continue; //try to send again
            }
            UHD_ASSERT_THROW(ret == ssize_t(size()));
        }
        _claimer.release();
    }

    UHD_INLINE sptr get_new(const double timeout, size_t &index){
        if (not _claimer.claim_with_wait(timeout)) return sptr();
        index++; //advances the caller's buffer
        return make(this, _mem, _frame_size);
    }

private:
    void *_mem;
    int _sock_fd;
    size_t _frame_size;
    simple_claimer _claimer;
};

tcp_zero_copy::~tcp_zero_copy(void){
    /* NOP */
}

/***********************************************************************
 * Zero Copy TCP implementation with ASIO:
 *   This is the portable zero copy implementation for systems
 *   where a faster, platform specific solution is not available.
 *   However, it is not a true zero copy implementation as each
 *   send and recv requires a copy operation to/from userspace.
 **********************************************************************/
class tcp_zero_copy_asio_impl : public tcp_zero_copy{
public:
    typedef boost::shared_ptr<tcp_zero_copy_asio_impl> sptr;

    tcp_zero_copy_asio_impl(
        const std::string &addr,
        const std::string &port,
        const device_addr_t &hints
    ):
        _recv_frame_size(size_t(hints.cast<double>("recv_frame_size", DEFAULT_FRAME_SIZE))),
        _num_recv_frames(size_t(hints.cast<double>("num_recv_frames", DEFAULT_NUM_FRAMES))),
        _send_frame_size(size_t(hints.cast<double>("send_frame_size", DEFAULT_FRAME_SIZE))),
        _num_send_frames(size_t(hints.cast<double>("num_send_frames", DEFAULT_NUM_FRAMES))),
        _recv_buffer_pool(buffer_pool::make(_num_recv_frames, _recv_frame_size)),
        _send_buffer_pool(buffer_pool::make(_num_send_frames, _send_frame_size)),
        _next_recv_buff_index(0), _next_send_buff_index(0)
    {
        UHD_LOG << boost::format("Creating tcp transport for %s %s") % addr % port << std::endl;

        //resolve the address
        asio::ip::tcp::resolver resolver(_io_service);
        asio::ip::tcp::resolver::query query(asio::ip::tcp::v4(), addr, port);
        asio::ip::tcp::endpoint receiver_endpoint = *resolver.resolve(query);

        //create, open, and connect the socket
        _socket.reset(new asio::ip::tcp::socket(_io_service));
        _socket->connect(receiver_endpoint);
        _sock_fd = _socket->native_handle();

        //packets go out ASAP
        asio::ip::tcp::no_delay option(true);
        _socket->set_option(option);

        //allocate re-usable managed receive buffers
        for (size_t i = 0; i < get_num_recv_frames(); i++){
            _mrb_pool.push_back(boost::make_shared<tcp_zero_copy_asio_mrb>(
                _recv_buffer_pool->at(i), _sock_fd, get_recv_frame_size()
            ));
        }

        //allocate re-usable managed send buffers
        for (size_t i = 0; i < get_num_send_frames(); i++){
            _msb_pool.push_back(boost::make_shared<tcp_zero_copy_asio_msb>(
                _send_buffer_pool->at(i), _sock_fd, get_send_frame_size()
            ));
        }
    }

    /*******************************************************************
     * Receive implementation:
     * Block on the managed buffer's get call and advance the index.
     ******************************************************************/
    managed_recv_buffer::sptr get_recv_buff(double timeout){
        if (_next_recv_buff_index == _num_recv_frames) _next_recv_buff_index = 0;
        return _mrb_pool[_next_recv_buff_index]->get_new(timeout, _next_recv_buff_index);
    }

    size_t get_num_recv_frames(void) const {return _num_recv_frames;}
    size_t get_recv_frame_size(void) const {return _recv_frame_size;}

    /*******************************************************************
     * Send implementation:
     * Block on the managed buffer's get call and advance the index.
     ******************************************************************/
    managed_send_buffer::sptr get_send_buff(double timeout){
        if (_next_send_buff_index == _num_send_frames) _next_send_buff_index = 0;
        return _msb_pool[_next_send_buff_index]->get_new(timeout, _next_send_buff_index);
    }

    size_t get_num_send_frames(void) const {return _num_send_frames;}
    size_t get_send_frame_size(void) const {return _send_frame_size;}

private:
    //memory management -> buffers and fifos
    const size_t _recv_frame_size, _num_recv_frames;
    const size_t _send_frame_size, _num_send_frames;
    buffer_pool::sptr _recv_buffer_pool, _send_buffer_pool;
    std::vector<boost::shared_ptr<tcp_zero_copy_asio_msb> > _msb_pool;
    std::vector<boost::shared_ptr<tcp_zero_copy_asio_mrb> > _mrb_pool;
    size_t _next_recv_buff_index, _next_send_buff_index;

    //asio guts -> socket and service
    asio::io_service        _io_service;
    boost::shared_ptr<asio::ip::tcp::socket> _socket;
    int                     _sock_fd;
};

/***********************************************************************
 * TCP zero copy make function
 **********************************************************************/
zero_copy_if::sptr tcp_zero_copy::make(
    const std::string &addr,
    const std::string &port,
    const device_addr_t &hints
){
    zero_copy_if::sptr xport;
    xport.reset(new tcp_zero_copy_asio_impl(addr, port, hints));
    while (xport->get_recv_buff(0.0)){} //flush
    return xport;
}

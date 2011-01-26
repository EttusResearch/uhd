//
// Copyright 2010-2011 Ettus Research LLC
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

#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/transport/udp_simple.hpp> //mtu
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/transport/buffer_pool.hpp>
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/warning.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * Constants
 **********************************************************************/
//Define this to the the boost async io calls to perform receive.
//Otherwise, get_recv_buff uses a blocking receive with timeout.
#define USE_ASIO_ASYNC_RECV

//Define this to the the boost async io calls to perform send.
//Otherwise, the commit callback uses a blocking send.
//#define USE_ASIO_ASYNC_SEND

//By default, this buffer is sized insufficiently small.
//For peformance, this buffer should be 10s of megabytes.
static const size_t MIN_RECV_SOCK_BUFF_SIZE = size_t(10e3);

//Large buffers cause more underflow at high rates.
//Perhaps this is due to the kernel scheduling,
//but may change with host-based flow control.
static const size_t MIN_SEND_SOCK_BUFF_SIZE = size_t(10e3);

//The number of async frames to allocate for each send and recv:
//The non-async recv can have a very large number of recv frames
//because the CPU overhead is independent of the number of frames.
#ifdef USE_ASIO_ASYNC_RECV
static const size_t DEFAULT_NUM_RECV_FRAMES = 32;
#else
static const size_t DEFAULT_NUM_RECV_FRAMES = MIN_RECV_SOCK_BUFF_SIZE/udp_simple::mtu;
#endif

//The non-async send only ever requires a single frame
//because the buffer will be committed before a new get.
#ifdef USE_ASIO_ASYNC_SEND
static const size_t DEFAULT_NUM_SEND_FRAMES = 32;
#else
static const size_t DEFAULT_NUM_SEND_FRAMES = MIN_SEND_SOCK_BUFF_SIZE/udp_simple::mtu;
#endif

//The number of service threads to spawn for async ASIO:
//A single concurrent thread for io_service seems to be the fastest.
//Threads are disabled when no async implementations are enabled.
#if defined(USE_ASIO_ASYNC_RECV) || defined(USE_ASIO_ASYNC_SEND)
static const size_t CONCURRENCY_HINT = 1;
#else
static const size_t CONCURRENCY_HINT = 0;
#endif

/***********************************************************************
 * Zero Copy UDP implementation with ASIO:
 *   This is the portable zero copy implementation for systems
 *   where a faster, platform specific solution is not available.
 *   However, it is not a true zero copy implementation as each
 *   send and recv requires a copy operation to/from userspace.
 **********************************************************************/
class udp_zero_copy_asio_impl : public udp_zero_copy, public boost::enable_shared_from_this<udp_zero_copy_asio_impl> {
public:
    typedef boost::shared_ptr<udp_zero_copy_asio_impl> sptr;

    udp_zero_copy_asio_impl(
        const std::string &addr,
        const std::string &port,
        const device_addr_t &hints
    ):
        _recv_frame_size(size_t(hints.cast<double>("recv_frame_size", udp_simple::mtu))),
        _num_recv_frames(size_t(hints.cast<double>("num_recv_frames", DEFAULT_NUM_RECV_FRAMES))),
        _send_frame_size(size_t(hints.cast<double>("send_frame_size", udp_simple::mtu))),
        _num_send_frames(size_t(hints.cast<double>("num_send_frames", DEFAULT_NUM_SEND_FRAMES))),
        _concurrency_hint(hints.cast<size_t>("concurrency_hint", CONCURRENCY_HINT)),
        _io_service(_concurrency_hint)
    {
        //std::cout << boost::format("Creating udp transport for %s %s") % addr % port << std::endl;

        //resolve the address
        asio::ip::udp::resolver resolver(_io_service);
        asio::ip::udp::resolver::query query(asio::ip::udp::v4(), addr, port);
        asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);

        //create, open, and connect the socket
        _socket = new asio::ip::udp::socket(_io_service);
        _socket->open(asio::ip::udp::v4());
        _socket->connect(receiver_endpoint);
        _sock_fd = _socket->native();
    }

    ~udp_zero_copy_asio_impl(void){
        delete _work; //allow io_service run to complete
        _thread_group.join_all(); //wait for service threads to exit
        delete _socket;
    }

    void init(void){
        //allocate all recv frames and release them to begin xfers
        _pending_recv_buffs = pending_buffs_type::make(_num_recv_frames);
        _recv_buffer_pool = buffer_pool::make(_num_recv_frames, _recv_frame_size);
        for (size_t i = 0; i < _num_recv_frames; i++){
            release(_recv_buffer_pool->at(i));
        }

        //allocate all send frames and push them into the fifo
        _pending_send_buffs = pending_buffs_type::make(_num_send_frames);
        _send_buffer_pool = buffer_pool::make(_num_send_frames, _send_frame_size);
        for (size_t i = 0; i < _num_send_frames; i++){
            handle_send(_send_buffer_pool->at(i));
        }

        //spawn the service threads that will run the io service
        _work = new asio::io_service::work(_io_service); //new work to delete later
        for (size_t i = 0; i < _concurrency_hint; i++) _thread_group.create_thread(
            boost::bind(&udp_zero_copy_asio_impl::service, this)
        );
    }

    void service(void){
        set_thread_priority_safe();
        _io_service.run();
    }

    //get size for internal socket buffer
    template <typename Opt> size_t get_buff_size(void) const{
        Opt option;
        _socket->get_option(option);
        return option.value();
    }

    //set size for internal socket buffer
    template <typename Opt> size_t resize_buff(size_t num_bytes){
        Opt option(num_bytes);
        _socket->set_option(option);
        return get_buff_size<Opt>();
    }

    //! handle a recv callback -> push the filled memory into the fifo
    UHD_INLINE void handle_recv(void *mem, size_t len){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        _pending_recv_buffs->push_with_wait(boost::asio::buffer(mem, len));
    }

    ////////////////////////////////////////////////////////////////////
    #ifdef USE_ASIO_ASYNC_RECV
    ////////////////////////////////////////////////////////////////////
    //! pop a filled recv buffer off of the fifo and bind with the release callback
    managed_recv_buffer::sptr get_recv_buff(double timeout){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        asio::mutable_buffer buff;
        if (_pending_recv_buffs->pop_with_timed_wait(buff, timeout)){
            return managed_recv_buffer::make_safe(
                buff, boost::bind(
                    &udp_zero_copy_asio_impl::release,
                    shared_from_this(),
                    asio::buffer_cast<void*>(buff)
                )
            );
        }
        return managed_recv_buffer::sptr();
    }

    //! release a recv buffer -> start an async recv on the buffer
    void release(void *mem){
        _socket->async_receive(
            boost::asio::buffer(mem, this->get_recv_frame_size()),
            boost::bind(
                &udp_zero_copy_asio_impl::handle_recv,
                shared_from_this(), mem,
                asio::placeholders::bytes_transferred
            )
        );
    }

    ////////////////////////////////////////////////////////////////////
    #else /*USE_ASIO_ASYNC_RECV*/
    ////////////////////////////////////////////////////////////////////
    managed_recv_buffer::sptr get_recv_buff(double timeout){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        asio::mutable_buffer buff;

        //setup timeval for timeout
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = long(timeout*1e6);

        //setup rset for timeout
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(_sock_fd, &rset);

        //call select to perform timed wait and grab an available buffer with wait
        //if the condition is true, call receive and return the managed buffer
        if (
            ::select(_sock_fd+1, &rset, NULL, NULL, &tv) > 0 and
            _pending_recv_buffs->pop_with_timed_wait(buff, timeout)
        ){
            return managed_recv_buffer::make_safe(
                asio::buffer(
                    boost::asio::buffer_cast<void *>(buff),
                    _socket->receive(asio::buffer(buff))
                ),
                boost::bind(
                    &udp_zero_copy_asio_impl::release,
                    shared_from_this(),
                    asio::buffer_cast<void*>(buff)
                )
            );
        }
        return managed_recv_buffer::sptr();
    }

    void release(void *mem){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        handle_recv(mem, this->get_recv_frame_size());
    }

    ////////////////////////////////////////////////////////////////////
    #endif /*USE_ASIO_ASYNC_RECV*/
    ////////////////////////////////////////////////////////////////////

    size_t get_num_recv_frames(void) const {return _num_recv_frames;}
    size_t get_recv_frame_size(void) const {return _recv_frame_size;}

    //! handle a send callback -> push the emptied memory into the fifo
    UHD_INLINE void handle_send(void *mem){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        _pending_send_buffs->push_with_wait(boost::asio::buffer(mem, this->get_send_frame_size()));
    }

    //! pop an empty send buffer off of the fifo and bind with the commit callback
    managed_send_buffer::sptr get_send_buff(double timeout){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        asio::mutable_buffer buff;
        if (_pending_send_buffs->pop_with_timed_wait(buff, timeout)){
            return managed_send_buffer::make_safe(
                buff, boost::bind(
                    &udp_zero_copy_asio_impl::commit,
                    shared_from_this(),
                    asio::buffer_cast<void*>(buff), _1
                )
            );
        }
        return managed_send_buffer::sptr();
    }

    ////////////////////////////////////////////////////////////////////
    #ifdef USE_ASIO_ASYNC_SEND
    ////////////////////////////////////////////////////////////////////
    //! commit a send buffer -> start an async send on the buffer
    void commit(void *mem, size_t len){
        _socket->async_send(
            boost::asio::buffer(mem, len),
            boost::bind(
                &udp_zero_copy_asio_impl::handle_send,
                shared_from_this(), mem
            )
        );
    }

    ////////////////////////////////////////////////////////////////////
    #else /*USE_ASIO_ASYNC_SEND*/
    ////////////////////////////////////////////////////////////////////
    void commit(void *mem, size_t len){
        _socket->send(asio::buffer(mem, len));
        handle_send(mem);
    }

    ////////////////////////////////////////////////////////////////////
    #endif /*USE_ASIO_ASYNC_SEND*/
    ////////////////////////////////////////////////////////////////////

    size_t get_num_send_frames(void) const {return _num_send_frames;}
    size_t get_send_frame_size(void) const {return _send_frame_size;}

private:
    //memory management -> buffers and fifos
    boost::thread_group _thread_group;
    buffer_pool::sptr _send_buffer_pool, _recv_buffer_pool;
    typedef bounded_buffer<asio::mutable_buffer> pending_buffs_type;
    pending_buffs_type::sptr _pending_recv_buffs, _pending_send_buffs;
    const size_t _recv_frame_size, _num_recv_frames;
    const size_t _send_frame_size, _num_send_frames;

    //asio guts -> socket and service
    size_t                  _concurrency_hint;
    asio::io_service        _io_service;
    asio::ip::udp::socket   *_socket;
    asio::io_service::work  *_work;
    int                     _sock_fd;
};

/***********************************************************************
 * UDP zero copy make function
 **********************************************************************/
template<typename Opt> static void resize_buff_helper(
    udp_zero_copy_asio_impl::sptr udp_trans,
    const size_t target_size,
    const std::string &name
){
    size_t min_sock_buff_size = 0;
    if (name == "recv") min_sock_buff_size = MIN_RECV_SOCK_BUFF_SIZE;
    if (name == "send") min_sock_buff_size = MIN_SEND_SOCK_BUFF_SIZE;
    min_sock_buff_size = std::max(min_sock_buff_size, target_size);

    std::string help_message;
    #if defined(UHD_PLATFORM_LINUX)
        help_message = str(boost::format(
            "Please run: sudo sysctl -w net.core.%smem_max=%d\n"
        ) % ((name == "recv")?"r":"w") % min_sock_buff_size);
    #endif /*defined(UHD_PLATFORM_LINUX)*/

    //resize the buffer if size was provided
    if (target_size > 0){
        size_t actual_size = udp_trans->resize_buff<Opt>(target_size);
        if (target_size != actual_size) std::cout << boost::format(
            "Target %s sock buff size: %d bytes\n"
            "Actual %s sock buff size: %d bytes"
        ) % name % target_size % name % actual_size << std::endl;
        else std::cout << boost::format(
            "Current %s sock buff size: %d bytes"
        ) % name % actual_size << std::endl;
        if (actual_size < target_size) uhd::warning::post(str(boost::format(
            "The %s buffer is smaller than the requested size.\n"
            "The minimum requested buffer size is %d bytes.\n"
            "See the transport application notes on buffer resizing.\n%s"
        ) % name % min_sock_buff_size % help_message));
    }
    //otherwise, ensure that the buffer is at least the minimum size
    else if (udp_trans->get_buff_size<Opt>() < min_sock_buff_size){
        resize_buff_helper<Opt>(udp_trans, min_sock_buff_size, name);
    }
}

udp_zero_copy::sptr udp_zero_copy::make(
    const std::string &addr,
    const std::string &port,
    const device_addr_t &hints
){
    udp_zero_copy_asio_impl::sptr udp_trans(
        new udp_zero_copy_asio_impl(addr, port, hints)
    );

    //extract buffer size hints from the device addr
    size_t recv_buff_size = size_t(hints.cast<double>("recv_buff_size", 0.0));
    size_t send_buff_size = size_t(hints.cast<double>("send_buff_size", 0.0));

    //call the helper to resize send and recv buffers
    resize_buff_helper<asio::socket_base::receive_buffer_size>(udp_trans, recv_buff_size, "recv");
    resize_buff_helper<asio::socket_base::send_buffer_size>   (udp_trans, send_buff_size, "send");

    udp_trans->init(); //buffers resized -> call init() to use

    return udp_trans;
}

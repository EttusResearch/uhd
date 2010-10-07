//
// Copyright 2010 Ettus Research LLC
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
#include <uhd/utils/thread_priority.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/warning.hpp>
#include <boost/shared_array.hpp>
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
//enough buffering for half a second of samples at full rate on usrp2
static const size_t MIN_RECV_SOCK_BUFF_SIZE = size_t(4 * 25e6 * 0.5);

//Large buffers cause more underflow at high rates.
//Perhaps this is due to the kernel scheduling,
//but may change with host-based flow control.
static const size_t MIN_SEND_SOCK_BUFF_SIZE = size_t(10e3);

//the number of async frames to allocate for each send and recv
static const size_t DEFAULT_NUM_FRAMES = 32;

//a single concurrent thread for io_service seems to be the fastest
static const size_t CONCURRENCY_HINT = 1;

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
        _io_service(hints.cast<size_t>("concurrency_hint", CONCURRENCY_HINT)),
        _recv_frame_size(size_t(hints.cast<double>("recv_frame_size", udp_simple::mtu))),
        _num_recv_frames(size_t(hints.cast<double>("num_recv_frames", DEFAULT_NUM_FRAMES))),
        _send_frame_size(size_t(hints.cast<double>("send_frame_size", udp_simple::mtu))),
        _num_send_frames(size_t(hints.cast<double>("num_send_frames", DEFAULT_NUM_FRAMES)))
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
    }

    void init(void){
        //allocate all recv frames and release them to begin xfers
        _pending_recv_buffs = pending_buffs_type::make(_num_recv_frames);
        _recv_buffer = boost::shared_array<char>(new char[_num_recv_frames*_recv_frame_size]);
        for (size_t i = 0; i < _num_recv_frames; i++){
            release(_recv_buffer.get() + i*_recv_frame_size);
        }

        //allocate all send frames and push them into the fifo
        _pending_send_buffs = pending_buffs_type::make(_num_send_frames);
        _send_buffer = boost::shared_array<char>(new char[_num_send_frames*_send_frame_size]);
        for (size_t i = 0; i < _num_send_frames; i++){
            handle_send(_send_buffer.get() + i*_send_frame_size);
        }

        //spawn the service threads that will run the io service
        _work = new asio::io_service::work(_io_service); //new work to delete later
        for (size_t i = 0; i < CONCURRENCY_HINT; i++) _thread_group.create_thread(
            boost::bind(&udp_zero_copy_asio_impl::service, this)
        );
    }

    ~udp_zero_copy_asio_impl(void){
        delete _work; //allow io_service run to complete
        _thread_group.join_all(); //wait for service threads to exit
        delete _socket;
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

    size_t get_num_recv_frames(void) const {return _num_recv_frames;}
    size_t get_recv_frame_size(void) const {return _recv_frame_size;}

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

    size_t get_num_send_frames(void) const {return _num_send_frames;}
    size_t get_send_frame_size(void) const {return _send_frame_size;}

private:
    void service(void){
        set_thread_priority_safe();
        _io_service.run();
    }

    /*******************************************************************
     * The async send and receive callbacks
     ******************************************************************/

    //! handle a recv callback -> push the filled memory into the fifo
    void handle_recv(void *mem, size_t len){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        _pending_recv_buffs->push_with_wait(boost::asio::buffer(mem, len));
    }

    //! release a recv buffer -> start an async recv on the buffer
    void release(void *mem){
        _socket->async_receive(
            boost::asio::buffer(mem, _recv_frame_size),
            boost::bind(
                &udp_zero_copy_asio_impl::handle_recv,
                shared_from_this(), mem,
                asio::placeholders::bytes_transferred
            )
        );
    }

    //! handle a send callback -> push the emptied memory into the fifo
    void handle_send(void *mem){
        boost::this_thread::disable_interruption di; //disable because the wait can throw
        _pending_send_buffs->push_with_wait(boost::asio::buffer(mem, _send_frame_size));
    }

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

    //asio guts -> socket and service
    asio::ip::udp::socket   *_socket;
    asio::io_service        _io_service;
    asio::io_service::work  *_work;

    //memory management -> buffers and fifos
    boost::thread_group _thread_group;
    boost::shared_array<char> _send_buffer, _recv_buffer;
    typedef bounded_buffer<asio::mutable_buffer> pending_buffs_type;
    pending_buffs_type::sptr _pending_recv_buffs, _pending_send_buffs;
    const size_t _recv_frame_size, _num_recv_frames;
    const size_t _send_frame_size, _num_send_frames;
};

/***********************************************************************
 * UDP zero copy make function
 **********************************************************************/
template<typename Opt> static void resize_buff_helper(
    udp_zero_copy_asio_impl::sptr udp_trans,
    size_t target_size,
    const std::string &name
){
    size_t min_sock_buff_size = 0;
    if (name == "recv") min_sock_buff_size = MIN_RECV_SOCK_BUFF_SIZE;
    if (name == "send") min_sock_buff_size = MIN_SEND_SOCK_BUFF_SIZE;

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
        if (actual_size < target_size) uhd::print_warning(str(boost::format(
            "The %1% buffer is smaller than the requested size.\n"
            "The minimum recommended buffer size is %2% bytes.\n"
            "See the transport application notes on buffer resizing.\n"
            #if defined(UHD_PLATFORM_LINUX)
            "Please run: sudo sysctl -w net.core.rmem_max=%2%\n"
            #endif /*defined(UHD_PLATFORM_LINUX)*/
        ) % name % min_sock_buff_size));
    }

    //only enable on platforms that are happy with the large buffer resize
    #if defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
    //otherwise, ensure that the buffer is at least the minimum size
    else if (udp_trans->get_buff_size<Opt>() < min_sock_buff_size){
        resize_buff_helper<Opt>(udp_trans, min_sock_buff_size, name);
    }
    #endif /*defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)*/
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

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
#include <uhd/utils/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <iostream>

using namespace uhd::transport;

/***********************************************************************
 * Constants
 **********************************************************************/
//enough buffering for half a second of samples at full rate on usrp2
static const size_t MIN_RECV_SOCK_BUFF_SIZE = size_t(sizeof(boost::uint32_t) * 25e6 * 0.5);
//Large buffers cause more underflow at high rates.
//Perhaps this is due to the kernel scheduling,
//but may change with host-based flow control.
static const size_t MIN_SEND_SOCK_BUFF_SIZE = size_t(10e3);
static const double RECV_TIMEOUT = 0.1; //100 ms

/***********************************************************************
 * Zero Copy UDP implementation with ASIO:
 *   This is the portable zero copy implementation for systems
 *   where a faster, platform specific solution is not available.
 *   However, it is not a true zero copy implementation as each
 *   send and recv requires a copy operation to/from userspace.
 **********************************************************************/
class udp_zero_copy_impl:
    public phony_zero_copy_recv_if,
    public phony_zero_copy_send_if,
    public udp_zero_copy
{
public:
    typedef boost::shared_ptr<udp_zero_copy_impl> sptr;

    udp_zero_copy_impl(
        const std::string &addr,
        const std::string &port
    ):
        phony_zero_copy_recv_if(udp_simple::mtu),
        phony_zero_copy_send_if(udp_simple::mtu)
    {
        //std::cout << boost::format("Creating udp transport for %s %s") % addr % port << std::endl;

        // resolve the address
        boost::asio::ip::udp::resolver resolver(_io_service);
        boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), addr, port);
        boost::asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);

        // create, open, and connect the socket
        _socket = new boost::asio::ip::udp::socket(_io_service);
        _socket->open(boost::asio::ip::udp::v4());
        _socket->connect(receiver_endpoint);
        _sock_fd = _socket->native();
    }

    ~udp_zero_copy_impl(void){
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


    //The number of frames is approximately the buffer size divided by the max datagram size.
    //In reality, this is a phony zero-copy interface and the number of frames is infinite.
    //However, its sensible to advertise a frame count that is approximate to buffer size.
    //This way, the transport caller will have an idea about how much buffering to create.

    size_t get_num_recv_frames(void) const{
        return this->get_buff_size<boost::asio::socket_base::receive_buffer_size>()/udp_simple::mtu;
    }

    size_t get_num_send_frames(void) const{
        return this->get_buff_size<boost::asio::socket_base::send_buffer_size>()/udp_simple::mtu;
    }

private:
    boost::asio::ip::udp::socket   *_socket;
    boost::asio::io_service        _io_service;
    int                            _sock_fd;

    ssize_t recv(const boost::asio::mutable_buffer &buff){
        //setup timeval for timeout
        timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = int(RECV_TIMEOUT*1e6);

        //setup rset for timeout
        fd_set rset;
        FD_ZERO(&rset);
        FD_SET(_sock_fd, &rset);

        //call select to perform timed wait
        if (::select(_sock_fd+1, &rset, NULL, NULL, &tv) <= 0) return 0;

        return ::recv(
            _sock_fd,
            boost::asio::buffer_cast<char *>(buff),
            boost::asio::buffer_size(buff), 0
        );
    }

    ssize_t send(const boost::asio::const_buffer &buff){
        return ::send(
            _sock_fd,
            boost::asio::buffer_cast<const char *>(buff),
            boost::asio::buffer_size(buff), 0
        );
    }
};

/***********************************************************************
 * UDP zero copy make function
 **********************************************************************/
template<typename Opt> static void resize_buff_helper(
    udp_zero_copy_impl::sptr udp_trans,
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
        if (actual_size < target_size) std::cerr << boost::format(
            "Warning:\n"
            "    The %s buffer is smaller than the requested size.\n"
            "    The minimum recommended buffer size is %d bytes.\n"
            "    See the USRP2 application notes on buffer resizing.\n"
        ) % name % min_sock_buff_size << std::endl;
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
    size_t recv_buff_size,
    size_t send_buff_size
){
    udp_zero_copy_impl::sptr udp_trans(new udp_zero_copy_impl(addr, port));

    //call the helper to resize send and recv buffers
    resize_buff_helper<boost::asio::socket_base::receive_buffer_size>(udp_trans, recv_buff_size, "recv");
    resize_buff_helper<boost::asio::socket_base::send_buffer_size>   (udp_trans, send_buff_size, "send");

    return udp_trans;
}

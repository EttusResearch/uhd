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
#include <uhd/utils/assert.hpp>
#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/format.hpp>
#include <iostream>

using namespace uhd::transport;

/***********************************************************************
 * Constants
 **********************************************************************/
static const size_t MIN_SOCK_BUFF_SIZE = size_t(100e3);
static const size_t MAX_DGRAM_SIZE = 2048; //assume max size on send and recv
static const double RECV_TIMEOUT = 0.1; // 100 ms

/***********************************************************************
 * Zero Copy UDP implementation with ASIO:
 *   This is the portable zero copy implementation for systems
 *   where a faster, platform specific solution is not available.
 *   However, it is not a true zero copy implementation as each
 *   send and recv requires a copy operation to/from userspace.
 **********************************************************************/
class udp_zero_copy_impl:
    public virtual phony_zero_copy_recv_if,
    public virtual phony_zero_copy_send_if,
    public virtual udp_zero_copy
{
public:
    typedef boost::shared_ptr<udp_zero_copy_impl> sptr;

    //structors
    udp_zero_copy_impl(const std::string &addr, const std::string &port);
    ~udp_zero_copy_impl(void);

    //get size for internal socket buffer
    template <typename Opt> size_t get_buff_size(void){
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

private:
    boost::asio::ip::udp::socket   *_socket;
    boost::asio::io_service        _io_service;

    size_t recv(const boost::asio::mutable_buffer &buff){
        return _socket->receive(boost::asio::buffer(buff));
    }

    size_t send(const boost::asio::const_buffer &buff){
        return _socket->send(boost::asio::buffer(buff));
    }
};

udp_zero_copy_impl::udp_zero_copy_impl(const std::string &addr, const std::string &port):
    phony_zero_copy_recv_if(MAX_DGRAM_SIZE),
    phony_zero_copy_send_if(MAX_DGRAM_SIZE)
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

    // set recv timeout
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = size_t(RECV_TIMEOUT*1e6);
    UHD_ASSERT_THROW(setsockopt(
        _socket->native(),
        SOL_SOCKET, SO_RCVTIMEO,
        (const char *)&tv, sizeof(timeval)
    ) == 0);
}

udp_zero_copy_impl::~udp_zero_copy_impl(void){
    delete _socket;
}

/***********************************************************************
 * UDP zero copy make function
 **********************************************************************/
template<typename Opt> static inline void resize_buff_helper(
    udp_zero_copy_impl::sptr udp_trans,
    size_t target_size,
    const std::string &name
){
    //resize the buffer if size was provided
    if (target_size > 0){
        size_t actual_size = udp_trans->resize_buff<Opt>(target_size);
        if (target_size != actual_size) std::cout << boost::format(
            "Target %s buffer size: %d\n"
            "Actual %s byffer size: %d"
        ) % name % target_size % name % actual_size << std::endl;
    }

    //otherwise, ensure that the buffer is at least the minimum size
    else if (udp_trans->get_buff_size<Opt>() < MIN_SOCK_BUFF_SIZE){
        resize_buff_helper<Opt>(udp_trans, MIN_SOCK_BUFF_SIZE, name);
    }
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

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
 * Managed receive buffer implementation for udp zero-copy asio:
 **********************************************************************/
class managed_recv_buffer_impl : public managed_recv_buffer{
public:
    managed_recv_buffer_impl(const boost::asio::const_buffer &buff) : _buff(buff){
        /* NOP */
    }

    ~managed_recv_buffer_impl(void){
        /* NOP */
    }

    void done(void){
        /* NOP */
    }

private:
    const boost::asio::const_buffer &get(void) const{
        return _buff;
    }

    const boost::asio::const_buffer _buff;
};

/***********************************************************************
 * Managed send buffer implementation for udp zero-copy asio:
 **********************************************************************/
class managed_send_buffer_impl : public managed_send_buffer{
public:
    managed_send_buffer_impl(
        const boost::asio::mutable_buffer &buff,
        boost::asio::ip::udp::socket *socket
    ) : _buff(buff), _socket(socket){
        /* NOP */
    }

    ~managed_send_buffer_impl(void){
        /* NOP */
    }

    void done(size_t num_bytes){
        _socket->send(boost::asio::buffer(_buff, num_bytes));
    }

private:
    const boost::asio::mutable_buffer &get(void) const{
        return _buff;
    }

    const boost::asio::mutable_buffer _buff;
    boost::asio::ip::udp::socket      *_socket;
};

/***********************************************************************
 * Zero Copy UDP implementation with ASIO:
 *   This is the portable zero copy implementation for systems
 *   where a faster, platform specific solution is not available.
 *   However, it is not a true zero copy implementation as each
 *   send and recv requires a copy operation to/from userspace.
 **********************************************************************/
static const size_t max_buff_size = 2000; //assume max size on send and recv

class udp_zero_copy_impl : public udp_zero_copy{
public:
    //structors
    udp_zero_copy_impl(const std::string &addr, const std::string &port);
    ~udp_zero_copy_impl(void);

    //send/recv
    managed_recv_buffer::sptr get_recv_buff(void);
    managed_send_buffer::sptr get_send_buff(void);

    //resize
    size_t resize_recv_buff(size_t num_bytes){
        boost::asio::socket_base::receive_buffer_size option(num_bytes);
        _socket->set_option(option);
        _socket->get_option(option);
        return option.value();
    }
    size_t resize_send_buff(size_t num_bytes){
        boost::asio::socket_base::send_buffer_size option(num_bytes);
        _socket->set_option(option);
        _socket->get_option(option);
        return option.value();
    }

private:
    boost::asio::ip::udp::socket   *_socket;
    boost::asio::io_service        _io_service;

    //send and recv buffer memory (allocated once)
    boost::uint8_t _send_mem[max_buff_size], _recv_mem[max_buff_size];

    managed_send_buffer::sptr _send_buff;
};

udp_zero_copy_impl::udp_zero_copy_impl(const std::string &addr, const std::string &port){
    //std::cout << boost::format("Creating udp transport for %s %s") % addr % port << std::endl;

    // resolve the address
    boost::asio::ip::udp::resolver resolver(_io_service);
    boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), addr, port);
    boost::asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);

    // create, open, and connect the socket
    _socket = new boost::asio::ip::udp::socket(_io_service);
    _socket->open(boost::asio::ip::udp::v4());
    _socket->connect(receiver_endpoint);

    // create the managed send buff (just once)
    _send_buff = managed_send_buffer::sptr(new managed_send_buffer_impl(
        boost::asio::buffer(_send_mem, max_buff_size), _socket
    ));

    // set recv timeout
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 100*1000; //100 ms
    UHD_ASSERT_THROW(setsockopt(
        _socket->native(),
        SOL_SOCKET, SO_RCVTIMEO,
        (timeval *)&tv, sizeof(timeval)
    ) == 0);
}

udp_zero_copy_impl::~udp_zero_copy_impl(void){
    delete _socket;
}

managed_recv_buffer::sptr udp_zero_copy_impl::get_recv_buff(void){
    //call recv() with timeout option
    size_t num_bytes = _socket->receive(boost::asio::buffer(_recv_mem, max_buff_size));

    //create a new managed buffer to house the data
    return managed_recv_buffer::sptr(
        new managed_recv_buffer_impl(boost::asio::buffer(_recv_mem, num_bytes))
    );
}

managed_send_buffer::sptr udp_zero_copy_impl::get_send_buff(void){
    return _send_buff;
}

/***********************************************************************
 * UDP zero copy make function
 **********************************************************************/
udp_zero_copy::sptr udp_zero_copy::make(
    const std::string &addr,
    const std::string &port,
    size_t recv_buff_size,
    size_t send_buff_size
){
    boost::shared_ptr<udp_zero_copy_impl> udp_trans(new udp_zero_copy_impl(addr, port));

    //resize the recv buffer if size was provided
    if (recv_buff_size > 0){
        size_t actual_bytes = udp_trans->resize_recv_buff(recv_buff_size);
        if (recv_buff_size != actual_bytes) std::cout << boost::format(
            "Target recv buffer size: %d\n"
            "Actual recv byffer size: %d"
        ) % recv_buff_size % actual_bytes << std::endl;
    }

    //resize the send buffer if size was provided
    if (send_buff_size > 0){
        size_t actual_bytes = udp_trans->resize_send_buff(send_buff_size);
        if (send_buff_size != actual_bytes) std::cout << boost::format(
            "Target send buffer size: %d\n"
            "Actual send byffer size: %d"
        ) % send_buff_size % actual_bytes << std::endl;
    }

    return udp_trans;
}

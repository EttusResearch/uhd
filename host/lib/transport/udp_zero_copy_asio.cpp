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
#include <boost/cstdint.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <iostream>

using namespace uhd::transport;

/***********************************************************************
 * Smart buffer implementation for udp zerocopy none
 *
 * This smart buffer implemention houses a const buffer.
 * When the smart buffer is deleted, the buffer is freed.
 * The memory in the const buffer is allocated with new [],
 * and so the destructor frees the buffer with delete [].
 **********************************************************************/
class smart_buffer_impl : public smart_buffer{
public:
    smart_buffer_impl(const boost::asio::const_buffer &buff){
        _buff = buff;
    }

    ~smart_buffer_impl(void){
        delete [] boost::asio::buffer_cast<const boost::uint32_t *>(_buff);
    }

    const boost::asio::const_buffer &get(void) const{
        return _buff;
    }

private:
    boost::asio::const_buffer _buff;
};

/***********************************************************************
 * UDP zero copy implementation class
 *
 * This is the portable zero copy implementation for systems
 * where a faster, platform specific solution is not available.
 *
 * It uses boost asio udp sockets and the standard recv() class,
 * and in-fact, is not actually doing a zero-copy implementation.
 **********************************************************************/
class udp_zero_copy_impl : public udp_zero_copy{
public:
    //structors
    udp_zero_copy_impl(const std::string &addr, const std::string &port);
    ~udp_zero_copy_impl(void);

    //send/recv
    size_t send(const boost::asio::const_buffer &buff);
    smart_buffer::sptr recv(void);

private:
    boost::asio::ip::udp::socket   *_socket;
    boost::asio::io_service        _io_service;

    size_t get_recv_buff_size(void);
    void set_recv_buff_size(size_t);
};

udp_zero_copy_impl::udp_zero_copy_impl(const std::string &addr, const std::string &port){
    //std::cout << boost::format("Creating udp transport for %s %s") % addr % port << std::endl;

    // resolve the address
    boost::asio::ip::udp::resolver resolver(_io_service);
    boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), addr, port);
    boost::asio::ip::udp::endpoint receiver_endpoint = *resolver.resolve(query);

    // Create, open, and connect the socket
    _socket = new boost::asio::ip::udp::socket(_io_service);
    _socket->open(boost::asio::ip::udp::v4());
    _socket->connect(receiver_endpoint);

    // set the rx socket buffer size:
    // pick a huge size, and deal with whatever we get
    set_recv_buff_size(size_t(54321e3)); //some big number!
    size_t current_buff_size = get_recv_buff_size();
    std::cout << boost::format(
        "Current rx socket buffer size: %d\n"
    ) % current_buff_size;
    if (current_buff_size < size_t(.1e6)) std::cout << boost::format(
        "Adjust max rx socket buffer size (linux only):\n"
        "  sysctl -w net.core.rmem_max=VALUE\n"
    );
}

udp_zero_copy_impl::~udp_zero_copy_impl(void){
    delete _socket;
}

size_t udp_zero_copy_impl::send(const boost::asio::const_buffer &buff){
    return _socket->send(boost::asio::buffer(buff));
}

smart_buffer::sptr udp_zero_copy_impl::recv(void){
    size_t available = 0;

    //implement timeout through polling and sleeping
    boost::asio::deadline_timer timer(_socket->get_io_service());
    timer.expires_from_now(boost::posix_time::milliseconds(100));
    while (not ((available = _socket->available()) or timer.expires_from_now().is_negative())){
        boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }

    //allocate memory and create buffer
    boost::uint32_t *buff_mem = new boost::uint32_t[available/sizeof(boost::uint32_t)];
    boost::asio::mutable_buffer buff(buff_mem, available);

    //receive only if data is available
    if (available){
        _socket->receive(boost::asio::buffer(buff));
    }

    //create a new smart buffer to house the data
    return smart_buffer::sptr(new smart_buffer_impl(buff));
}

size_t udp_zero_copy_impl::get_recv_buff_size(void){
    boost::asio::socket_base::receive_buffer_size option;
    _socket->get_option(option);
    return option.value();
}

void udp_zero_copy_impl::set_recv_buff_size(size_t new_size){
    boost::asio::socket_base::receive_buffer_size option(new_size);
    _socket->set_option(option);
}

/***********************************************************************
 * UDP zero copy make function
 **********************************************************************/
udp_zero_copy::sptr udp_zero_copy::make(
    const std::string &addr, const std::string &port
){
    return sptr(new udp_zero_copy_impl(addr, port));
}

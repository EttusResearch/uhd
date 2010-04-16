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
 * Managed receive buffer implementation for udp zero-copy asio:
 *   Frees the memory held by the const buffer on done.
 **********************************************************************/
class managed_recv_buffer_impl : public managed_recv_buffer{
public:
    managed_recv_buffer_impl(const boost::asio::const_buffer &buff) : _buff(buff){
        _done = false;
    }

    ~managed_recv_buffer_impl(void){
        if (not _done) this->done();
    }

    void done(void){
        _done = true;
        delete [] boost::asio::buffer_cast<const boost::uint32_t *>(_buff);
    }

private:
    const boost::asio::const_buffer &get(void){
        return _buff;
    }

    const boost::asio::const_buffer _buff;
    bool _done;
};

/***********************************************************************
 * Managed send buffer implementation for udp zero-copy asio:
 *   Sends and frees the memory held by the mutable buffer on done.
 **********************************************************************/
class managed_send_buffer_impl : public managed_send_buffer{
public:
    managed_send_buffer_impl(
        const boost::asio::mutable_buffer &buff,
        boost::asio::ip::udp::socket *socket
    ) : _buff(buff){
        _done = false;
        _socket = socket;
    }

    ~managed_send_buffer_impl(void){
        if (not _done) this->done(0);
    }

    void done(size_t num_bytes){
        _done = true;
        boost::uint32_t *mem = boost::asio::buffer_cast<boost::uint32_t *>(_buff);
        _socket->send(boost::asio::buffer(mem, num_bytes));
        delete [] mem;
    }

private:
    const boost::asio::mutable_buffer &get(void){
        return _buff;
    }

    const boost::asio::mutable_buffer _buff;
    boost::asio::ip::udp::socket      *_socket;
    bool _done;
};

/***********************************************************************
 * Zero Copy UDP implementation with ASIO:
 *   This is the portable zero copy implementation for systems
 *   where a faster, platform specific solution is not available.
 *   However, it is not a true zero copy implementation as each
 *   send and recv requires a copy operation to/from userspace.
 **********************************************************************/
class udp_zero_copy_impl : public udp_zero_copy{
public:
    //structors
    udp_zero_copy_impl(const std::string &addr, const std::string &port);
    ~udp_zero_copy_impl(void);

    //send/recv
    managed_recv_buffer::sptr get_recv_buff(void);
    managed_send_buffer::sptr get_send_buff(void);

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

managed_recv_buffer::sptr udp_zero_copy_impl::get_recv_buff(void){
    //implement timeout through polling and sleeping
    size_t available = 0;
    boost::asio::deadline_timer timer(_socket->get_io_service());
    timer.expires_from_now(boost::posix_time::milliseconds(100));
    while (not ((available = _socket->available()) or timer.expires_from_now().is_negative())){
        boost::this_thread::sleep(boost::posix_time::milliseconds(1));
    }

    //receive only if data is available
    boost::uint32_t *buff_mem = new boost::uint32_t[available/sizeof(boost::uint32_t)];
    if (available){
        available = _socket->receive(boost::asio::buffer(buff_mem, available));
    }

    //create a new managed buffer to house the data
    return managed_recv_buffer::sptr(
        new managed_recv_buffer_impl(boost::asio::buffer(buff_mem, available))
    );
}

managed_send_buffer::sptr udp_zero_copy_impl::get_send_buff(void){
    boost::uint32_t *buff_mem = new boost::uint32_t[2000/sizeof(boost::uint32_t)];
    return managed_send_buffer::sptr(
        new managed_send_buffer_impl(boost::asio::buffer(buff_mem, 2000), _socket)
    );
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

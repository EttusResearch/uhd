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

#include <uhd/transport/udp.hpp>
#include <boost/format.hpp>
#include <iostream>

/***********************************************************************
 * UDP implementation class
 **********************************************************************/
class udp_impl : public uhd::transport::udp{
public:
    //structors
    udp_impl(const std::string &addr, const std::string &port, bool bcast);
    ~udp_impl(void);

    //send/recv
    size_t send(const std::vector<boost::asio::const_buffer> &buffs);
    size_t send(const boost::asio::const_buffer &buff);
    size_t recv(const boost::asio::mutable_buffer &buff);
    uhd::shared_iovec recv(void);

private:
    boost::asio::ip::udp::socket   *_socket;
    boost::asio::ip::udp::endpoint _receiver_endpoint;
    boost::asio::ip::udp::endpoint _sender_endpoint;
    boost::asio::io_service        _io_service;
};

/***********************************************************************
 * UDP public make function
 **********************************************************************/
uhd::transport::udp::sptr uhd::transport::udp::make(
    const std::string &addr,
    const std::string &port,
    bool bcast
){
    return uhd::transport::udp::sptr(new udp_impl(addr, port, bcast));
}

/***********************************************************************
 * UDP implementation methods
 **********************************************************************/
udp_impl::udp_impl(const std::string &addr, const std::string &port, bool bcast){
    //std::cout << boost::format("Creating udp transport for %s %s") % addr % port << std::endl;

    // resolve the address
    boost::asio::ip::udp::resolver resolver(_io_service);
    boost::asio::ip::udp::resolver::query query(boost::asio::ip::udp::v4(), addr, port);
    _receiver_endpoint = *resolver.resolve(query);

    // Create and open the socket
    _socket = new boost::asio::ip::udp::socket(_io_service);
    _socket->open(boost::asio::ip::udp::v4());

    if (bcast){
        // Allow broadcasting
        boost::asio::socket_base::broadcast option(true);
        _socket->set_option(option);
    }

}

udp_impl::~udp_impl(void){
    delete _socket;
}

size_t udp_impl::send(const std::vector<boost::asio::const_buffer> &buffs){
    return _socket->send_to(buffs, _receiver_endpoint);
}

size_t udp_impl::send(const boost::asio::const_buffer &buff){
    return _socket->send_to(boost::asio::buffer(buff), _receiver_endpoint);
}

size_t udp_impl::recv(const boost::asio::mutable_buffer &buff){
    if (_socket->available() == 0) return 0;
    return _socket->receive_from(boost::asio::buffer(buff), _sender_endpoint);
}

uhd::shared_iovec udp_impl::recv(void){
    //allocate a buffer for the number of bytes available (could be zero)
    uhd::shared_iovec iov(_socket->available());
    //call recv only if data is available
    if (iov.len != 0){
        _socket->receive_from(
            boost::asio::buffer(iov.base, iov.len),
            _sender_endpoint
        );
    }
    return iov;
}

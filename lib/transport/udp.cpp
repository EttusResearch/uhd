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
#include <boost/assign/list_of.hpp>
#include <iostream>

uhd::transport::udp::udp(const std::string &addr, const std::string &port, bool bcast){
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

uhd::transport::udp::~udp(void){
    delete _socket;
}

void uhd::transport::udp::send(const std::vector<boost::asio::const_buffer> &buffs){
    _socket->send_to(buffs, _receiver_endpoint);
}

void uhd::transport::udp::send(const boost::asio::const_buffer &buff){
    std::vector<boost::asio::const_buffer> buffs = boost::assign::list_of(buff);
    send(buffs);
}

boost::asio::const_buffer uhd::transport::udp::recv(void){
    size_t len = 0;
    //recv if data is available
    if (_socket->available()){
        len = _socket->receive_from(
            boost::asio::buffer(_recv_buff, sizeof(_recv_buff)),
            _sender_endpoint
        );
    }
    //return the buffer with the received length
    return boost::asio::buffer(_recv_buff, len);
}

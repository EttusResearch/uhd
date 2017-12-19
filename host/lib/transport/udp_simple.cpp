//
// Copyright 2010-2011,2014 Ettus Research LLC
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
#include <uhd/transport/udp_simple.hpp>
#include <uhd/utils/log.hpp>
#include <boost/format.hpp>

using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * UDP simple implementation: connected and broadcast
 **********************************************************************/
class udp_simple_impl : public udp_simple{
public:
    udp_simple_impl(
        const std::string &addr, const std::string &port, bool bcast, bool connect
    ):_connected(connect){
        UHD_LOG << boost::format("Creating udp transport for %s %s") % addr % port << std::endl;

        //resolve the address
        asio::ip::udp::resolver resolver(_io_service);
        asio::ip::udp::resolver::query query(asio::ip::udp::v4(), addr, port);
        _send_endpoint = *resolver.resolve(query);

        //create and open the socket
        _socket = socket_sptr(new asio::ip::udp::socket(_io_service));
        _socket->open(asio::ip::udp::v4());

        //allow broadcasting
        _socket->set_option(asio::socket_base::broadcast(bcast));

        //connect the socket
        if (connect) _socket->connect(_send_endpoint);

    }

    size_t send(const asio::const_buffer &buff){
        if (_connected) return _socket->send(asio::buffer(buff));
        return _socket->send_to(asio::buffer(buff), _send_endpoint);
    }

    size_t recv(const asio::mutable_buffer &buff, double timeout){
        if (not wait_for_recv_ready(_socket->native_handle(), timeout)) return 0;
        return _socket->receive_from(asio::buffer(buff), _recv_endpoint);
    }

    std::string get_recv_addr(void){
        return _recv_endpoint.address().to_string();
    }

private:
    bool                    _connected;
    asio::io_service        _io_service;
    socket_sptr             _socket;
    asio::ip::udp::endpoint _send_endpoint;
    asio::ip::udp::endpoint _recv_endpoint;
};

udp_simple::~udp_simple(void){
    /* NOP */
}

/***********************************************************************
 * UDP public make functions
 **********************************************************************/
udp_simple::sptr udp_simple::make_connected(
    const std::string &addr, const std::string &port
){
    return sptr(new udp_simple_impl(addr, port, false, true /* no bcast, connect */));
}

udp_simple::sptr udp_simple::make_broadcast(
    const std::string &addr, const std::string &port
){
    return sptr(new udp_simple_impl(addr, port, true, false /* bcast, no connect */));
}

/***********************************************************************
 * Simple UART over UDP
 **********************************************************************/
#include <boost/thread/thread.hpp>
class udp_simple_uart_impl : public uhd::uart_iface{
public:
    udp_simple_uart_impl(udp_simple::sptr udp){
        _udp = udp;
        _len = 0;
        _off = 0;
        this->write_uart(""); //send an empty packet to init
    }

    void write_uart(const std::string &buf){
        _udp->send(asio::buffer(buf));
    }

    std::string read_uart(double timeout){
        std::string line;
        const boost::system_time exit_time = boost::get_system_time() + boost::posix_time::milliseconds(long(timeout*1000));
        do{
            //drain anything in current buffer
            while (_off < _len){
                const char ch = _buf[_off++];
                _line += ch;
                if (ch == '\n')
                {
                    line.swap(_line);
                    return line;
                }
            }

            //recv a new packet into the buffer
            _len = _udp->recv(asio::buffer(_buf), std::max((exit_time - boost::get_system_time()).total_milliseconds()/1000., 0.0));
            _off = 0;

        } while (_len != 0);
        return line;
    }

private:
    udp_simple::sptr _udp;
    size_t _len, _off;
    uint8_t _buf[udp_simple::mtu];
    std::string _line;
};

uhd::uart_iface::sptr udp_simple::make_uart(sptr udp){
    return uart_iface::sptr(new udp_simple_uart_impl(udp));
}

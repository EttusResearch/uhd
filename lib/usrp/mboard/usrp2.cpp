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

#include <uhd/usrp/mboard/usrp2.hpp>
#include "usrp2_fw_common.h"
#include <uhd/device.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <boost/utility.hpp>
#include <boost/lexical_cast.hpp>
#include <netinet/in.h>

using namespace uhd;
using namespace uhd::usrp::mboard;
using boost::asio::ip::udp;

/***********************************************************************
 * Wrapper for the udp transport
 **********************************************************************/
class udp_transport : boost::noncopyable{
public:
    udp_transport(const std::string &addr, const std::string &port, bool bcast = false){
        //std::cout << boost::format("Creating udp transport for %s %s") % addr % port << std::endl;

        // resolve the address
        udp::resolver resolver(_io_service);
        udp::resolver::query query(udp::v4(), addr, port);
        _receiver_endpoint = *resolver.resolve(query);

        // Create and open the socket
        _socket = new udp::socket(_io_service);
        _socket->open(udp::v4());

        if (bcast){
            // Allow broadcasting
            boost::asio::socket_base::broadcast option(true);
            _socket->set_option(option);
        }

    }

    ~udp_transport(void){
        delete _socket;
    }

    void send(const device::send_args_t &buffs){
        _socket->send_to(buffs, _receiver_endpoint);
    }

    void send(const void *buff, size_t len){
        _socket->send_to(boost::asio::buffer(buff, len), _receiver_endpoint);
    }

    void recv(const device::recv_args_t &handler){
        // make sure that bytes are available (crappy timeout 100 ms)
        for (size_t i = 0; i < 10; i++){
            if (_socket->available()) break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(10));
        }

        // receive the bytes and call the handler
        udp::endpoint sender_endpoint;
        while (_socket->available()){
            size_t len = _socket->receive_from(
                boost::asio::buffer(_recv_buff, sizeof(_recv_buff)),
                sender_endpoint
            );
            bool done = handler(boost::asio::buffer(_recv_buff, len));
            if (done) return;
        }
    }

private:
    udp::socket *_socket;
    udp::endpoint _receiver_endpoint;
    boost::asio::io_service _io_service;
    uint8_t _recv_buff[1500];
};

/***********************************************************************
 * Discovery over the udp transport
 **********************************************************************/
std::vector<device_addr_t> usrp2::discover(const device_addr_t &hint){
    std::vector<device_addr_t> usrp2_addrs;

    //create a udp transport to communicate
    std::string ctrl_port = boost::lexical_cast<std::string>(USRP2_UDP_CTRL_PORT);
    udp_transport trans(hint.udp_args.addr, ctrl_port, true);

    //send a hello control packet
    usrp2_ctrl_data_t data;
    data.id = htonl(USRP2_CTRL_ID_HELLO);
    trans.send(&data, sizeof(data));

    //TODO start a thread to listen and sleep for timeout

    return usrp2_addrs;
}

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
#include <uhd/transport/udp.hpp>
#include "usrp2_fw_common.h"
#include <uhd/device.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include <netinet/in.h>

using namespace uhd::usrp::mboard;

/***********************************************************************
 * Discovery over the udp transport
 **********************************************************************/
std::vector<uhd::device_addr_t> usrp2::discover(const device_addr_t &hint){
    std::vector<uhd::device_addr_t> usrp2_addrs;

    //create a udp transport to communicate
    std::string ctrl_port = boost::lexical_cast<std::string>(USRP2_UDP_CTRL_PORT);
    uhd::transport::udp udp_transport(hint.udp_args.addr, ctrl_port, true);

    //send a hello control packet
    usrp2_ctrl_data_t ctrl_data_out;
    ctrl_data_out.id = htonl(USRP2_CTRL_ID_HELLO);
    udp_transport.send(boost::asio::buffer(&ctrl_data_out, sizeof(ctrl_data_out)));

    //loop and recieve until the time is up
    size_t num_timeouts = 0;
    while(true){
        boost::asio::const_buffer buff = udp_transport.recv();
        //std::cout << boost::asio::buffer_size(buff) << "\n";
        if (boost::asio::buffer_size(buff) < sizeof(usrp2_ctrl_data_t)){
            //sleep a little so we dont burn cpu
            if (num_timeouts++ > 50) break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }else{
            //handle the received data
            const usrp2_ctrl_data_t *ctrl_data_in = boost::asio::buffer_cast<const usrp2_ctrl_data_t *>(buff);
            switch(ntohl(ctrl_data_in->id)){
            case USRP2_CTRL_ID_HELLO:
                //make a boost asio ipv4 with the raw addr in host byte order
                boost::asio::ip::address_v4 ip_addr(ntohl(ctrl_data_in->data.discovery_addrs.ip_addr));
                std::cout << "hello " << ip_addr.to_string() << "\n";
                break;
            }
        }
    }

    return usrp2_addrs;
}

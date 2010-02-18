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
#include <uhd/device.hpp>
#include <uhd/transport/udp.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <netinet/in.h>
#include "usrp2/usrp2_impl.hpp"

using namespace uhd::usrp::mboard;

/***********************************************************************
 * Discovery over the udp transport
 **********************************************************************/
uhd::device_addrs_t usrp2::discover(const device_addr_t &hint){
    device_addrs_t usrp2_addrs;

    //create a udp transport to communicate
    //TODO if an addr is not provided, search all interfaces?
    std::string ctrl_port = boost::lexical_cast<std::string>(USRP2_UDP_CTRL_PORT);
    uhd::transport::udp udp_transport(hint["addr"], ctrl_port, true);

    //send a hello control packet
    usrp2_ctrl_data_t ctrl_data_out;
    ctrl_data_out.id = htonl(USRP2_CTRL_ID_GIVE_ME_YOUR_IP_ADDR_BRO);
    udp_transport.send(boost::asio::buffer(&ctrl_data_out, sizeof(ctrl_data_out)));

    //loop and recieve until the time is up
    size_t num_timeouts = 0;
    while(true){
        uhd::shared_iovec iov = udp_transport.recv();
        //std::cout << boost::asio::buffer_size(buff) << "\n";
        if (iov.len < sizeof(usrp2_ctrl_data_t)){
            //sleep a little so we dont burn cpu
            if (num_timeouts++ > 50) break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }else{
            //handle the received data
            const usrp2_ctrl_data_t *ctrl_data_in = reinterpret_cast<const usrp2_ctrl_data_t *>(iov.base);
            switch(ntohl(ctrl_data_in->id)){
            case USRP2_CTRL_ID_THIS_IS_MY_IP_ADDR_DUDE:
                //make a boost asio ipv4 with the raw addr in host byte order
                boost::asio::ip::address_v4 ip_addr(ntohl(ctrl_data_in->data.ip_addr));
                device_addr_t new_addr;
                new_addr["name"] = "USRP2";
                new_addr["type"] = "udp";
                new_addr["addr"] = ip_addr.to_string();
                usrp2_addrs.push_back(new_addr);
                break;
            }
        }
    }

    return usrp2_addrs;
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2::usrp2(const device_addr_t &device_addr){
    //create a control transport
    uhd::transport::udp::sptr ctrl_transport(
        new uhd::transport::udp(
            device_addr["addr"],
            boost::lexical_cast<std::string>(USRP2_UDP_CTRL_PORT)
        )
    );

    //create a data transport
    uhd::transport::udp::sptr data_transport(
        new uhd::transport::udp(
            device_addr["addr"],
            boost::lexical_cast<std::string>(USRP2_UDP_DATA_PORT)
        )
    );

    //create the usrp2 implementation guts
    _impl = usrp2_impl::sptr(
        new usrp2_impl(ctrl_transport, data_transport)
    );
}

usrp2::~usrp2(void){
    /* NOP */
}

/***********************************************************************
 * Get Properties
 **********************************************************************/
void usrp2::get(const wax::obj &key, wax::obj &val){
    return wax::cast<usrp2_impl::sptr>(_impl)->get(key, val);
}

/***********************************************************************
 * Set Properties
 **********************************************************************/
void usrp2::set(const wax::obj &key, const wax::obj &val){
    return wax::cast<usrp2_impl::sptr>(_impl)->set(key, val);
}

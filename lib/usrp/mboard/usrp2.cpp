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
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <netinet/in.h>
#include "usrp2_fw_common.h"

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
    //initialize the transports for udp
    _udp_ctrl_transport = uhd::transport::udp::sptr(
        new uhd::transport::udp(
            device_addr["addr"],
            boost::lexical_cast<std::string>(USRP2_UDP_CTRL_PORT)
        )
    );
    _udp_data_transport = uhd::transport::udp::sptr(
        new uhd::transport::udp(
            device_addr["addr"],
            boost::lexical_cast<std::string>(USRP2_UDP_DATA_PORT)
        )
    );
    //grab the dboard ids over the control line
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_GIVE_ME_YOUR_DBOARD_IDS_BRO);
    usrp2_ctrl_data_t in_data = _ctrl_send_and_recv(out_data);
    //TODO assert the control data id response
    std::cout << boost::format("rx id 0x%.2x, tx id 0x%.2x")
        % ntohs(in_data.data.dboard_ids.rx_id)
        % ntohs(in_data.data.dboard_ids.tx_id) << std::endl;
    //TODO setup the dboard manager with the dboard ids
}

usrp2::~usrp2(void){
    /* NOP */
}

/***********************************************************************
 * Transactions
 **********************************************************************/
template <class T> T usrp2::_ctrl_send_and_recv(const T &out_data){
    //fill in the seq number and send
    T out_copy = out_data;
    out_copy.seq = htonl(++_ctrl_seq_num);
    _udp_ctrl_transport->send(boost::asio::buffer(&out_copy, sizeof(T)));

    //loop and recieve until the time is up
    size_t num_timeouts = 0;
    while(true){
        uhd::shared_iovec iov = _udp_ctrl_transport->recv();
        if (iov.len < sizeof(T)){
            //sleep a little so we dont burn cpu
            if (num_timeouts++ > 50) break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }else{
            //handle the received data
            T in_data = *reinterpret_cast<const T *>(iov.base);
            if (ntohl(in_data.seq) == _ctrl_seq_num){
                return in_data;
            }
            //didnt get seq, continue on...
        }
    }
    throw std::runtime_error("usrp2 no control response");
}

/***********************************************************************
 * Get Properties
 **********************************************************************/
void usrp2::get(const wax::obj &, wax::obj &){
    
}

/***********************************************************************
 * Set Properties
 **********************************************************************/
void usrp2::set(const wax::obj &, const wax::obj &){
    
}

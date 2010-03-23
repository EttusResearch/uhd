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

#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <uhd/utils.hpp>
#include <iostream>
#include "usrp2_impl.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

STATIC_BLOCK(register_usrp2_device){
    device::register_device(&usrp2::discover, &usrp2::make);
}

/***********************************************************************
 * Discovery over the udp transport
 **********************************************************************/
uhd::device_addrs_t usrp2::discover(const device_addr_t &hint){
    device_addrs_t usrp2_addrs;

    if (not hint.has_key("addr")) return usrp2_addrs;

    //create a udp transport to communicate
    //TODO if an addr is not provided, search all interfaces?
    std::string ctrl_port = boost::lexical_cast<std::string>(USRP2_UDP_CTRL_PORT);
    udp_simple::sptr udp_transport = udp_simple::make_broadcast(
        hint["addr"], ctrl_port
    );

    //send a hello control packet
    usrp2_ctrl_data_t ctrl_data_out;
    ctrl_data_out.id = htonl(USRP2_CTRL_ID_GIVE_ME_YOUR_IP_ADDR_BRO);
    udp_transport->send(boost::asio::buffer(&ctrl_data_out, sizeof(ctrl_data_out)));

    //loop and recieve until the timeout
    while(true){
        usrp2_ctrl_data_t ctrl_data_in;
        size_t len = udp_transport->recv(asio::buffer(&ctrl_data_in, sizeof(ctrl_data_in)));
        //std::cout << len << "\n";
        if (len >= sizeof(usrp2_ctrl_data_t)){
            //handle the received data
            switch(ntohl(ctrl_data_in.id)){
            case USRP2_CTRL_ID_THIS_IS_MY_IP_ADDR_DUDE:
                //make a boost asio ipv4 with the raw addr in host byte order
                boost::asio::ip::address_v4 ip_addr(ntohl(ctrl_data_in.data.ip_addr));
                device_addr_t new_addr;
                new_addr["name"] = "USRP2";
                new_addr["transport"] = "udp";
                new_addr["addr"] = ip_addr.to_string();
                usrp2_addrs.push_back(new_addr);
                //dont break here, it will exit the while loop
                //just continue on to the next loop iteration
            }
        }
        if (len == 0) break; //timeout
    }

    return usrp2_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
template <class T> std::string num2str(T num){
    return boost::lexical_cast<std::string>(num);
}

device::sptr usrp2::make(const device_addr_t &device_addr){
    //create a control transport
    udp_simple::sptr ctrl_transport = udp_simple::make_connected(
        device_addr["addr"], num2str(USRP2_UDP_CTRL_PORT)
    );

    //create a data transport
    udp_zero_copy::sptr data_transport = udp_zero_copy::make(
        device_addr["addr"], num2str(USRP2_UDP_DATA_PORT)
    );

    //create the usrp2 implementation guts
    return device::sptr(
        new usrp2_impl(ctrl_transport, data_transport)
    );
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_impl::usrp2_impl(
    udp_simple::sptr ctrl_transport,
    udp_zero_copy::sptr data_transport
){
    _ctrl_transport = ctrl_transport;
    _data_transport = data_transport;

    //load the allowed decim/interp rates
    //_USRP2_RATES = range(4, 128+1, 1) + range(130, 256+1, 2) + range(260, 512+1, 4)
    _allowed_decim_and_interp_rates.clear();
    for (size_t i = 4; i <= 128; i+=1){
        _allowed_decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 130; i <= 256; i+=2){
        _allowed_decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 260; i <= 512; i+=4){
        _allowed_decim_and_interp_rates.push_back(i);
    }

    //init the mboard
    mboard_init();

    //init the ddc
    init_ddc_config();

    //init the duc
    init_duc_config();

    //initialize the clock configuration
    init_clock_config();

    //init the tx and rx dboards (do last)
    dboard_init();

    //init the send and recv io
    io_init();

}

usrp2_impl::~usrp2_impl(void){
    /* NOP */
}

/***********************************************************************
 * Misc Access Methods
 **********************************************************************/
double usrp2_impl::get_master_clock_freq(void){
    return 100e6;
}

/***********************************************************************
 * Control Send/Recv
 **********************************************************************/
usrp2_ctrl_data_t usrp2_impl::ctrl_send_and_recv(const usrp2_ctrl_data_t &out_data){
    boost::mutex::scoped_lock lock(_ctrl_mutex);

    //fill in the seq number and send
    usrp2_ctrl_data_t out_copy = out_data;
    out_copy.seq = htonl(++_ctrl_seq_num);
    _ctrl_transport->send(boost::asio::buffer(&out_copy, sizeof(usrp2_ctrl_data_t)));

    //loop until we get the packet or timeout
    while(true){
        usrp2_ctrl_data_t in_data;
        size_t len = _ctrl_transport->recv(asio::buffer(&in_data, sizeof(in_data)));
        if (len >= sizeof(usrp2_ctrl_data_t) and ntohl(in_data.seq) == _ctrl_seq_num){
            return in_data;
        }
        if (len == 0) break; //timeout
        //didnt get seq or bad packet, continue looking...
    }
    throw std::runtime_error("usrp2 no control response");
}

/***********************************************************************
 * Device Properties
 **********************************************************************/
void usrp2_impl::get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<device_prop_t>()){
    case DEVICE_PROP_NAME:
        val = std::string("usrp2 device");
        return;

    case DEVICE_PROP_MBOARD:
        ASSERT_THROW(_mboards.has_key(name));
        val = _mboards[name]->get_link();
        return;

    case DEVICE_PROP_MBOARD_NAMES:
        val = prop_names_t(_mboards.get_keys());
        return;

    case DEVICE_PROP_MAX_RX_SAMPLES:
        val = size_t(_max_rx_samples_per_packet);
        return;

    case DEVICE_PROP_MAX_TX_SAMPLES:
        val = size_t(_max_tx_samples_per_packet);
        return;

    }
}

void usrp2_impl::set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("Cannot set in usrp2 device");
}

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

#include "usrp2_impl.hpp"
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/usrp/device_props.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/static.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp> //htonl and ntohl
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

UHD_STATIC_BLOCK(register_usrp2_device){
    device::register_device(&usrp2::find, &usrp2::make);
}

/***********************************************************************
 * Discovery over the udp transport
 **********************************************************************/
uhd::device_addrs_t usrp2::find(const device_addr_t &hint){
    device_addrs_t usrp2_addrs;

    //if no address was specified, send a broadcast on each interface
    if (not hint.has_key("addr")){
        BOOST_FOREACH(const if_addrs_t &if_addrs, get_if_addrs()){
            //avoid the loopback device
            if (if_addrs.inet == asio::ip::address_v4::loopback().to_string()) continue;

            //create a new hint with this broadcast address
            device_addr_t new_hint = hint;
            new_hint["addr"] = if_addrs.bcast;

            //call discover with the new hint and append results
            device_addrs_t new_usrp2_addrs = usrp2::find(new_hint);
            usrp2_addrs.insert(usrp2_addrs.begin(),
                new_usrp2_addrs.begin(), new_usrp2_addrs.end()
            );
        }
        return usrp2_addrs;
    }

    //create a udp transport to communicate
    std::string ctrl_port = boost::lexical_cast<std::string>(USRP2_UDP_CTRL_PORT);
    udp_simple::sptr udp_transport = udp_simple::make_broadcast(
        hint["addr"], ctrl_port
    );

    //send a hello control packet
    usrp2_ctrl_data_t ctrl_data_out;
    ctrl_data_out.proto_ver = htonl(USRP2_PROTO_VERSION);
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
    _data_transport = data_transport;

    //make a new interface for usrp2 stuff
    _iface = usrp2_iface::make(ctrl_transport);

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
        UHD_ASSERT_THROW(name == "");
        val = _mboard_proxy->get_link();
        return;

    case DEVICE_PROP_MBOARD_NAMES:
        val = prop_names_t(1, "");
        return;

    case DEVICE_PROP_MAX_RX_SAMPLES:
        val = size_t(_max_rx_samples_per_packet);
        return;

    case DEVICE_PROP_MAX_TX_SAMPLES:
        val = size_t(_max_tx_samples_per_packet);
        return;

    default: UHD_THROW_PROP_WRITE_ONLY();
    }
}

void usrp2_impl::set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_READ_ONLY();
}

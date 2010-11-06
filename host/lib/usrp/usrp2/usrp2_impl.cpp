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
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/usrp/device_props.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/warning.hpp>
#include <uhd/utils/algorithm.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp> //htonl and ntohl
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;
namespace asio = boost::asio;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
template <class T> std::string num2str(T num){
    return boost::lexical_cast<std::string>(num);
}

/***********************************************************************
 * Discovery over the udp transport
 **********************************************************************/
static uhd::device_addrs_t usrp2_find(const device_addr_t &hint){
    device_addrs_t usrp2_addrs;

    //return an empty list of addresses when type is set to non-usrp2
    if (hint.has_key("type") and hint["type"] != "usrp2") return usrp2_addrs;

    //if no address was specified, send a broadcast on each interface
    if (not hint.has_key("addr")){
        BOOST_FOREACH(const if_addrs_t &if_addrs, get_if_addrs()){
            //avoid the loopback device
            if (if_addrs.inet == asio::ip::address_v4::loopback().to_string()) continue;

            //create a new hint with this broadcast address
            device_addr_t new_hint = hint;
            new_hint["addr"] = if_addrs.bcast;

            //call discover with the new hint and append results
            device_addrs_t new_usrp2_addrs = usrp2_find(new_hint);
            usrp2_addrs.insert(usrp2_addrs.begin(),
                new_usrp2_addrs.begin(), new_usrp2_addrs.end()
            );
        }
        return usrp2_addrs;
    }

    //if there are multiple addresses, just return good, dont test
    std::vector<std::string> addrs = std::split_string(hint["addr"]);
    if (addrs.size() > 1){
        device_addr_t new_addr;
        new_addr["type"] = "usrp2";
        new_addr["addr"] = hint["addr"];
        usrp2_addrs.push_back(new_addr);
        return usrp2_addrs;
    }

    //create a udp transport to communicate
    std::string ctrl_port = boost::lexical_cast<std::string>(USRP2_UDP_CTRL_PORT);
    udp_simple::sptr udp_transport = udp_simple::make_broadcast(
        hint["addr"], ctrl_port
    );

    //send a hello control packet
    usrp2_ctrl_data_t ctrl_data_out;
    ctrl_data_out.proto_ver = htonl(USRP2_FW_COMPAT_NUM);
    ctrl_data_out.id = htonl(USRP2_CTRL_ID_WAZZUP_BRO);
    udp_transport->send(boost::asio::buffer(&ctrl_data_out, sizeof(ctrl_data_out)));

    //loop and recieve until the timeout
    boost::uint8_t usrp2_ctrl_data_in_mem[udp_simple::mtu]; //allocate max bytes for recv
    const usrp2_ctrl_data_t *ctrl_data_in = reinterpret_cast<const usrp2_ctrl_data_t *>(usrp2_ctrl_data_in_mem);
    while(true){
        size_t len = udp_transport->recv(asio::buffer(usrp2_ctrl_data_in_mem));
        //std::cout << len << "\n";
        if (len > offsetof(usrp2_ctrl_data_t, data) and ntohl(ctrl_data_in->id) == USRP2_CTRL_ID_WAZZUP_DUDE){
            //make a boost asio ipv4 with the raw addr in host byte order
            boost::asio::ip::address_v4 ip_addr(ntohl(ctrl_data_in->data.ip_addr));
            device_addr_t new_addr;
            new_addr["type"] = "usrp2";
            new_addr["addr"] = ip_addr.to_string();
            //Attempt to read the name from the EEPROM and perform filtering.
            //This operation can throw due to COMPAT mismatch. That is OK.
            //We will allow the device to be found and the COMPAT mismatch
            //will be thrown as an exception in the factory function.
            try{
                mboard_eeprom_t mb_eeprom = usrp2_iface::make(
                    udp_simple::make_connected(new_addr["addr"], num2str(USRP2_UDP_CTRL_PORT))
                )->mb_eeprom;
                new_addr["name"] = mb_eeprom["name"];
                new_addr["serial"] = mb_eeprom["serial"];
                if (
                    (not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
                    (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
                ){
                    usrp2_addrs.push_back(new_addr);
                }
            }
            catch(const std::exception &e){
                uhd::warning::post(
                    std::string("Ignoring discovered device\n")
                    + e.what()
                );
            }
            //dont break here, it will exit the while loop
            //just continue on to the next loop iteration
        }
        if (len == 0) break; //timeout
    }

    return usrp2_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr usrp2_make(const device_addr_t &device_addr){

    //create a ctrl and data transport for each address
    std::vector<udp_simple::sptr> ctrl_transports;
    std::vector<zero_copy_if::sptr> data_transports;

    BOOST_FOREACH(const std::string &addr, std::split_string(device_addr["addr"])){
        ctrl_transports.push_back(udp_simple::make_connected(
            addr, num2str(USRP2_UDP_CTRL_PORT)
        ));
        data_transports.push_back(udp_zero_copy::make(
            addr, num2str(USRP2_UDP_DATA_PORT), device_addr
        ));
    }

    //create the usrp2 implementation guts
    return device::sptr(
        new usrp2_impl(ctrl_transports, data_transports, device_addr)
    );
}

UHD_STATIC_BLOCK(register_usrp2_device){
    device::register_device(&usrp2_find, &usrp2_make);
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_impl::usrp2_impl(
    std::vector<udp_simple::sptr> ctrl_transports,
    std::vector<zero_copy_if::sptr> data_transports,
    const device_addr_t &flow_control_hints
):
    _data_transports(data_transports)
{
    //setup rx otw type
    _rx_otw_type.width = 16;
    _rx_otw_type.shift = 0;
    _rx_otw_type.byteorder = uhd::otw_type_t::BO_BIG_ENDIAN;

    //setup tx otw type
    _tx_otw_type.width = 16;
    _tx_otw_type.shift = 0;
    _tx_otw_type.byteorder = uhd::otw_type_t::BO_BIG_ENDIAN;

    //!!!!! set the otw type here before continuing, its used below

    //create a new mboard handler for each control transport
    for(size_t i = 0; i < ctrl_transports.size(); i++){
        _mboards.push_back(usrp2_mboard_impl::sptr(new usrp2_mboard_impl(
            i, ctrl_transports[i], data_transports[i],
            this->get_max_recv_samps_per_packet(),
            flow_control_hints
        )));
        //use an empty name when there is only one mboard
        std::string name = (ctrl_transports.size() > 1)? boost::lexical_cast<std::string>(i) : "";
        _mboard_dict[name] = _mboards.back();
    }

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
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<device_prop_t>()){
    case DEVICE_PROP_NAME:
        if (_mboards.size() > 1) val = std::string("usrp2 mimo device");
        else                     val = std::string("usrp2 device");
        return;

    case DEVICE_PROP_MBOARD:
        val = _mboard_dict[key.name]->get_link();
        return;

    case DEVICE_PROP_MBOARD_NAMES:
        val = prop_names_t(_mboard_dict.keys());
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void usrp2_impl::set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_SET_ERROR();
}

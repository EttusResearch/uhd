//
// Copyright 2010-2011 Ettus Research LLC
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
#include <boost/algorithm/string.hpp> //for split
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp> //htonl and ntohl
#include <iostream>
#include <vector>

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

//! separate indexed device addresses into a vector of device addresses
device_addrs_t sep_indexed_dev_addrs(const device_addr_t &dev_addr){
    //------------ support old deprecated way and print warning --------
    if (dev_addr.has_key("addr") and not dev_addr["addr"].empty()){
        std::vector<std::string> addrs; boost::split(addrs, dev_addr["addr"], boost::is_any_of(" "));
        if (addrs.size() > 1){
            device_addr_t fixed_dev_addr = dev_addr;
            fixed_dev_addr.pop("addr");
            for (size_t i = 0; i < addrs.size(); i++){
                fixed_dev_addr[str(boost::format("addr%d") % i)] = addrs[i];
            }
            uhd::warning::post(
                "addr = <space separated list of ip addresses> is deprecated.\n"
                "To address a multi-device, use multiple <key><index> = <val>.\n"
                "See the USRP-NXXX application notes. Two device example:\n"
                "    addr0 = 192.168.10.2\n"
                "    addr1 = 192.168.10.3\n"
            );
            return sep_indexed_dev_addrs(fixed_dev_addr);
        }
    }
    //------------------------------------------------------------------
    device_addrs_t dev_addrs;
    BOOST_FOREACH(const std::string &key, dev_addr.keys()){
        boost::cmatch matches;
        if (not boost::regex_match(key.c_str(), matches, boost::regex("^(\\D+)(\\d*)$"))){
            throw std::runtime_error("unknown key format: " + key);
        }
        std::string key_part(matches[1].first, matches[1].second);
        std::string num_part(matches[2].first, matches[2].second);
        size_t num = (num_part.empty())? 0 : boost::lexical_cast<size_t>(num_part);
        dev_addrs.resize(std::max(num+1, dev_addrs.size()));
        dev_addrs[num][key_part] = dev_addr[key];
    }
    return dev_addrs;
}

//! combine a vector in device addresses into an indexed device address
device_addr_t combine_dev_addr_vector(const device_addrs_t &dev_addrs){
    device_addr_t dev_addr;
    for (size_t i = 0; i < dev_addrs.size(); i++){
        BOOST_FOREACH(const std::string &key, dev_addrs[i].keys()){
            dev_addr[str(boost::format("%s%d") % key % i)] = dev_addrs[i][key];
        }
    }
    return dev_addr;
}

/***********************************************************************
 * Discovery over the udp transport
 **********************************************************************/
static device_addrs_t usrp2_find(const device_addr_t &hint_){
    //handle the multi-device discovery
    device_addrs_t hints = sep_indexed_dev_addrs(hint_);
    if (hints.size() > 1){
        device_addrs_t found_devices;
        BOOST_FOREACH(const device_addr_t &hint_i, hints){
            device_addrs_t found_devices_i = usrp2_find(hint_i);
            if (found_devices_i.size() != 1) throw std::runtime_error(str(boost::format(
                "Could not resolve device hint \"%s\" to a single device."
            ) % hint_i.to_string()));
            found_devices.push_back(found_devices_i[0]);
        }
        return device_addrs_t(1, combine_dev_addr_vector(found_devices));
    }

    //initialize the hint for a single device case
    UHD_ASSERT_THROW(hints.size() <= 1);
    hints.resize(1); //in case it was empty
    device_addr_t hint = hints[0];
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
            //This operation can throw due to compatibility mismatch.
            //In this case, the discovered device will be ignored.
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

    //setup the dsp transport hints (default to a large recv buff)
    device_addr_t dsp_xport_hints = device_addr;
    if (not dsp_xport_hints.has_key("recv_buff_size")){
        //only enable on platforms that are happy with the large buffer resize
        #if defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)
            //set to half-a-second of buffering at max rate
            dsp_xport_hints["recv_buff_size"] = "50e6";
        #endif /*defined(UHD_PLATFORM_LINUX) || defined(UHD_PLATFORM_WIN32)*/
    }

    //create a ctrl and data transport for each address
    std::vector<udp_simple::sptr> ctrl_transports;
    std::vector<zero_copy_if::sptr> data_transports;
    std::vector<zero_copy_if::sptr> err0_transports;
    const device_addrs_t device_addrs = sep_indexed_dev_addrs(device_addr);

    BOOST_FOREACH(const device_addr_t &dev_addr_i, device_addrs){
        ctrl_transports.push_back(udp_simple::make_connected(
            dev_addr_i["addr"], num2str(USRP2_UDP_CTRL_PORT)
        ));
        data_transports.push_back(udp_zero_copy::make(
            dev_addr_i["addr"], num2str(USRP2_UDP_DSP0_PORT), dsp_xport_hints
        ));
        err0_transports.push_back(udp_zero_copy::make(
            dev_addr_i["addr"], num2str(USRP2_UDP_ERR0_PORT), device_addr_t()
        ));
    }

    //create the usrp2 implementation guts
    return device::sptr(new usrp2_impl(
        ctrl_transports, data_transports, err0_transports, device_addrs
    ));
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
    std::vector<zero_copy_if::sptr> err0_transports,
    const device_addrs_t &device_args
):
    _data_transports(data_transports),
    _err0_transports(err0_transports)
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
    for(size_t i = 0; i < device_args.size(); i++){
        _mboards.push_back(usrp2_mboard_impl::sptr(new usrp2_mboard_impl(
            i, ctrl_transports[i], data_transports[i],
            err0_transports[i], device_args[i],
            this->get_max_recv_samps_per_packet()
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
        if (_mboards.size() > 1) val = std::string("USRP2/N Series multi-device");
        else                     val = std::string("USRP2/N Series device");
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

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

#include "usrp_e_impl.hpp"
#include <uhd/usrp/device_props.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/static.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
namespace fs = boost::filesystem;

UHD_STATIC_BLOCK(register_usrp_e_device){
    device::register_device(&usrp_e::find, &usrp_e::make);
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
static std::string abs_path(const std::string &file_path){
    return fs::system_complete(fs::path(file_path)).file_string();
}

/***********************************************************************
 * Discovery
 **********************************************************************/
device_addrs_t usrp_e::find(const device_addr_t &hint){
    device_addrs_t usrp_e_addrs;

    //device node not provided, assume its 0
    if (not hint.has_key("node")){
        device_addr_t new_addr = hint;
        new_addr["node"] = "/dev/usrp_e0";
        return usrp_e::find(new_addr);
    }

    //use the given device node name
    if (fs::exists(hint["node"])){
        device_addr_t new_addr;
        new_addr["name"] = "USRP-E";
        new_addr["node"] = abs_path(hint["node"]);
        usrp_e_addrs.push_back(new_addr);
    }

    return usrp_e_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
device::sptr usrp_e::make(const device_addr_t &device_addr){
    return sptr(new usrp_e_impl(device_addr["node"]));
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp_e_impl::usrp_e_impl(const std::string &node){
    std::cout << boost::format("Opening USRP-E on %s") % node << std::endl;

    //setup various interfaces into hardware
    _iface = usrp_e_iface::make(node);
    _clock_ctrl = clock_ctrl::make(_iface);
    _codec_ctrl = codec_ctrl::make(_iface);

    //initialize the mboard
    mboard_init();

    //initialize the dboards
    dboard_init();

    //initialize the dsps
    rx_ddc_init();
    tx_duc_init();
}

usrp_e_impl::~usrp_e_impl(void){
    /* NOP */
}

/***********************************************************************
 * Device Get
 **********************************************************************/
void usrp_e_impl::get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<device_prop_t>()){
    case DEVICE_PROP_NAME:
        val = std::string("usrp-e device");
        return;

    case DEVICE_PROP_MBOARD:
        UHD_ASSERT_THROW(name == "");
        val = _mboard_proxy->get_link();
        return;

    case DEVICE_PROP_MBOARD_NAMES:
        val = prop_names_t(1, ""); //vector of size 1 with empty string
        return;

    case DEVICE_PROP_MAX_RX_SAMPLES:
        val = size_t(_max_num_samples);
        return;

    case DEVICE_PROP_MAX_TX_SAMPLES:
        val = size_t(_max_num_samples);
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * Device Set
 **********************************************************************/
void usrp_e_impl::set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_SET_ERROR();
}

/***********************************************************************
 * Device IO (TODO)
 **********************************************************************/
size_t usrp_e_impl::send(
    const boost::asio::const_buffer &,
    const uhd::tx_metadata_t &,
    const io_type_t &
){
    if (true){
        throw std::runtime_error(str(boost::format("usrp-e send: cannot handle type \"%s\"") % ""));
    }
    return 0;
}

size_t usrp_e_impl::recv(
    const boost::asio::mutable_buffer &,
    uhd::rx_metadata_t &,
    const io_type_t &
){
    if (true){
        throw std::runtime_error(str(boost::format("usrp-e recv: cannot handle type \"%s\"") % ""));
    }
    return 0;
}

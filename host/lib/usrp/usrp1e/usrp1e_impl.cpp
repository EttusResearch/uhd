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

#include <uhd/utils.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include "usrp1e_impl.hpp"
#include <fcntl.h> //open
#include <sys/ioctl.h> //ioctl

using namespace uhd;
using namespace uhd::usrp;
namespace fs = boost::filesystem;

STATIC_BLOCK(register_usrp1e_device){
    device::register_device(&usrp1e::discover, &usrp1e::make);
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
device_addrs_t usrp1e::discover(const device_addr_t &device_addr){
    device_addrs_t usrp1e_addrs;

    //if a node was provided, use it and only it
    if (device_addr.has_key("node")){
        if (not fs::exists(device_addr["node"])) return usrp1e_addrs;
        device_addr_t new_addr;
        new_addr["name"] = "USRP1E";
        new_addr["node"] = abs_path(device_addr["node"]);
        usrp1e_addrs.push_back(new_addr);
    }

    //otherwise look for a few nodes at small indexes
    else{
        for(size_t i = 0; i < 5; i++){
            std::string node = str(boost::format("/dev/usrp1_e%d") % i);
            if (not fs::exists(node)) continue;
            device_addr_t new_addr;
            new_addr["name"] = "USRP1E";
            new_addr["node"] = abs_path(node);
            usrp1e_addrs.push_back(new_addr);
        }
    }

    return usrp1e_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
device::sptr usrp1e::make(const device_addr_t &device_addr){
    std::string node = device_addr["node"];
    int node_fd = open(node.c_str(), 0);
    if (node_fd < 0){
        throw std::runtime_error(str(
            boost::format("Failed to open %s") % node
        ));
    }
    return sptr(new usrp1e_impl(node_fd));
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp1e_impl::usrp1e_impl(int node_fd){
    _node_fd = node_fd;

    //initialize the mboard
    mboard_init();

    //initialize the dboards
    dboard_init();

    //initialize the dsps
    rx_ddc_init();
    tx_duc_init();
}

usrp1e_impl::~usrp1e_impl(void){
    /* NOP */
}

/***********************************************************************
 * Misc Methods
 **********************************************************************/
void usrp1e_impl::ioctl(int request, void *mem){
    if (::ioctl(_node_fd, request, mem) < 0){
        throw std::runtime_error(str(
            boost::format("ioctl failed with request %d") % request
        ));
    }
}

/***********************************************************************
 * Device Get
 **********************************************************************/
void usrp1e_impl::get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<device_prop_t>()){
    case DEVICE_PROP_NAME:
        val = std::string("usrp1e device");
        return;

    case DEVICE_PROP_MBOARD:
        ASSERT_THROW(name == "");
        val = _mboard_proxy->get_link();
        return;

    case DEVICE_PROP_MBOARD_NAMES:
        val = prop_names_t(1, ""); //vector of size 1 with empty string
        return;

    case DEVICE_PROP_MAX_RX_SAMPLES:
        val = size_t(0); //TODO
        return;

    case DEVICE_PROP_MAX_TX_SAMPLES:
        val = size_t(0); //TODO
        return;

    }
}

/***********************************************************************
 * Device Set
 **********************************************************************/
void usrp1e_impl::set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("Cannot set in usrp1e device");
}

/***********************************************************************
 * Device IO (TODO)
 **********************************************************************/
size_t usrp1e_impl::send(const boost::asio::const_buffer &, const uhd::tx_metadata_t &, const std::string &){return 0;}
size_t usrp1e_impl::recv(const boost::asio::mutable_buffer &, uhd::rx_metadata_t &, const std::string &){return 0;}

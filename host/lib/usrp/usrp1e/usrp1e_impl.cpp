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

using namespace uhd;
using namespace uhd::usrp;

STATIC_BLOCK(register_usrp1e_device){
    device::register_device(&usrp1e::discover, &usrp1e::make);
}

/***********************************************************************
 * Helper Functions
 **********************************************************************/
static bool file_exists(const std::string &file_path){
    return boost::filesystem::exists(file_path);
}

/***********************************************************************
 * Discovery
 **********************************************************************/
device_addrs_t usrp1e::discover(const device_addr_t &device_addr){
    device_addrs_t usrp1e_addrs;

    //if a node was provided, use it and only it
    if (device_addr.has_key("node")){
        if (not file_exists(device_addr["node"])) return usrp1e_addrs;
        device_addr_t new_addr;
        new_addr["name"] = "USRP1E";
        new_addr["node"] = device_addr["node"];
        usrp1e_addrs.push_back(new_addr);
    }

    //otherwise look for a few nodes at small indexes
    else{
        for(size_t i = 0; i < 5; i++){
            std::string node = str(boost::format("/dev/usrp1_e%d") % i);
            if (not file_exists(node)) continue;
            device_addr_t new_addr;
            new_addr["name"] = "USRP1E";
            new_addr["node"] = node;
            usrp1e_addrs.push_back(new_addr);
        }
    }

    return usrp1e_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
device::sptr usrp1e::make(const device_addr_t &){
    throw std::runtime_error("not implemented yet");
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp1e_impl::usrp1e_impl(void){
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
 * Device Get
 **********************************************************************/
void usrp1e_impl::get(const wax::obj &, wax::obj &){
    
}

/***********************************************************************
 * Device Set
 **********************************************************************/
void usrp1e_impl::set(const wax::obj &, const wax::obj &){
    
}

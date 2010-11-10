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

#include "usrp_e100_impl.hpp"
#include "usrp_e100_regs.hpp"
#include <uhd/usrp/device_props.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/images.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <iostream>
#include <fstream>

using namespace uhd;
using namespace uhd::usrp;
namespace fs = boost::filesystem;

/***********************************************************************
 * Helper Functions
 **********************************************************************/
static std::string abs_path(const std::string &file_path){
    return fs::system_complete(fs::path(file_path)).file_string();
}

/***********************************************************************
 * Discovery
 **********************************************************************/
static device_addrs_t usrp_e100_find(const device_addr_t &hint){
    device_addrs_t usrp_e100_addrs;

    //return an empty list of addresses when type is set to non-usrp-e
    if (hint.has_key("type") and hint["type"] != "usrp-e") return usrp_e100_addrs;

    //device node not provided, assume its 0
    if (not hint.has_key("node")){
        device_addr_t new_addr = hint;
        new_addr["node"] = "/dev/usrp_e1000";
        return usrp_e100_find(new_addr);
    }

    //use the given device node name
    if (fs::exists(hint["node"])){
        device_addr_t new_addr;
        new_addr["type"] = "usrp-e";
        new_addr["node"] = abs_path(hint["node"]);
        usrp_e100_addrs.push_back(new_addr);
    }

    return usrp_e100_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr usrp_e100_make(const device_addr_t &device_addr){

    //setup the main interface into fpga
    std::string node = device_addr["node"];
    std::cout << boost::format("Opening USRP-E on %s") % node << std::endl;
    usrp_e100_iface::sptr iface = usrp_e100_iface::make(node);

    //------------------------------------------------------------------
    //-- Handle the FPGA loading...
    //-- The image can be confimed as already loaded when:
    //--   1) The compatibility number matches.
    //--   2) The hash in the hash-file matches.
    //------------------------------------------------------------------
    static const char *hash_file_path = "/tmp/usrp_e100100_hash";

    //extract the fpga path for usrp-e
    std::string usrp_e100_fpga_image = find_image_path(
        device_addr.has_key("fpga")? device_addr["fpga"] : "usrp_e100100_fpga.bin"
    );

    //calculate a hash of the fpga file
    size_t fpga_hash = 0;
    {
        std::ifstream file(usrp_e100_fpga_image.c_str());
        if (not file.good()) throw std::runtime_error(
            "cannot open fpga file for read: " + usrp_e100_fpga_image
        );
        do{
            boost::hash_combine(fpga_hash, file.get());
        } while (file.good());
        file.close();
    }

    //read the compatibility number
    boost::uint16_t fpga_compat_num = iface->peek16(UE_REG_MISC_COMPAT);

    //read the hash in the hash-file
    size_t loaded_hash = 0;
    try{std::ifstream(hash_file_path) >> loaded_hash;}catch(...){}

    //if not loaded: load the fpga image and write the hash-file
    if (fpga_compat_num != USRP_E_COMPAT_NUM or loaded_hash != fpga_hash){
        iface.reset();
        usrp_e100_load_fpga(usrp_e100_fpga_image);
        std::cout << boost::format("re-Opening USRP-E on %s") % node << std::endl;
        iface = usrp_e100_iface::make(node);
        try{std::ofstream(hash_file_path) << fpga_hash;}catch(...){}
    }

    //check that the compatibility is correct
    fpga_compat_num = iface->peek16(UE_REG_MISC_COMPAT);
    if (fpga_compat_num != USRP_E_COMPAT_NUM){
        throw std::runtime_error(str(boost::format(
            "Expected fpga compatibility number 0x%x, but got 0x%x:\n"
            "The fpga build is not compatible with the host code build."
        ) % USRP_E_COMPAT_NUM % fpga_compat_num));
    }

    return device::sptr(new usrp_e100_impl(iface));
}

UHD_STATIC_BLOCK(register_usrp_e100_device){
    device::register_device(&usrp_e100_find, &usrp_e100_make);
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp_e100_impl::usrp_e100_impl(usrp_e100_iface::sptr iface): _iface(iface){

    //setup interfaces into hardware
    _clock_ctrl = usrp_e100_clock_ctrl::make(_iface);
    _codec_ctrl = usrp_e100_codec_ctrl::make(_iface);

    //initialize the mboard
    mboard_init();

    //initialize the dboards
    dboard_init();

    //initialize the dsps
    rx_ddc_init();
    tx_duc_init();

    //init the codec properties
    codec_init();

    //init the io send/recv
    io_init();

    //set default subdev specs
    this->mboard_set(MBOARD_PROP_RX_SUBDEV_SPEC, subdev_spec_t());
    this->mboard_set(MBOARD_PROP_TX_SUBDEV_SPEC, subdev_spec_t());
}

usrp_e100_impl::~usrp_e100_impl(void){
    /* NOP */
}

/***********************************************************************
 * Device Get
 **********************************************************************/
void usrp_e100_impl::get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<device_prop_t>()){
    case DEVICE_PROP_NAME:
        val = std::string("usrp-e device");
        return;

    case DEVICE_PROP_MBOARD:
        UHD_ASSERT_THROW(key.name == "");
        val = _mboard_proxy->get_link();
        return;

    case DEVICE_PROP_MBOARD_NAMES:
        val = prop_names_t(1, ""); //vector of size 1 with empty string
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * Device Set
 **********************************************************************/
void usrp_e100_impl::set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_SET_ERROR();
}

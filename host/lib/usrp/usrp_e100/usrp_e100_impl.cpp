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

#include "usrp_e100_impl.hpp"
#include "usrp_e100_regs.hpp"
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/device_props.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/images.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <fstream>

using namespace uhd;
using namespace uhd::usrp;
namespace fs = boost::filesystem;

/***********************************************************************
 * Discovery
 **********************************************************************/
static device_addrs_t usrp_e100_find(const device_addr_t &hint){
    device_addrs_t usrp_e100_addrs;

    //return an empty list of addresses when type is set to non-usrp-e
    if (hint.has_key("type") and hint["type"] != "e100") return usrp_e100_addrs;

    //device node not provided, assume its 0
    if (not hint.has_key("node")){
        device_addr_t new_addr = hint;
        new_addr["node"] = "/dev/usrp_e0";
        return usrp_e100_find(new_addr);
    }

    //use the given device node name
    if (fs::exists(hint["node"])){
        device_addr_t new_addr;
        new_addr["type"] = "usrp-e";
        new_addr["node"] = fs::system_complete(fs::path(hint["node"])).string();
        try{
            usrp_e100_iface::sptr iface = usrp_e100_iface::make(new_addr["node"]);
            new_addr["name"] = "";//FIXME for double open on next branch iface->mb_eeprom["name"];
            new_addr["serial"] = "";//FIXME for double open on next branch iface->mb_eeprom["serial"];
        }
        catch(const std::exception &e){
            new_addr["name"] = "";
            new_addr["serial"] = "";
        }
        if (
            (not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
            (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
        ){
            usrp_e100_addrs.push_back(new_addr);
        }
    }

    return usrp_e100_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static size_t hash_fpga_file(const std::string &file_path){
    size_t hash = 0;
    std::ifstream file(file_path.c_str());
    if (not file.good()) throw uhd::io_error("cannot open fpga file for read: " + file_path);
    while (file.good()) boost::hash_combine(hash, file.get());
    file.close();
    return hash;
}

static device::sptr usrp_e100_make(const device_addr_t &device_addr){

    //setup the main interface into fpga
    std::string node = device_addr["node"];
    UHD_MSG(status) << boost::format("Opening USRP-E on %s") % node << std::endl;
    usrp_e100_iface::sptr iface = usrp_e100_iface::make(node);

    //extract the fpga path for usrp-e
    std::string usrp_e100_fpga_image = find_image_path(device_addr.get("fpga", "usrp_e100_fpga.bin"));

    //compute a hash of the fpga file
    const boost::uint32_t file_hash = boost::uint32_t(hash_fpga_file(usrp_e100_fpga_image));

    //When the hash does not match:
    // - unload the iface to free the node
    // - load the fpga configuration file
    // - re-open the iface on the node
    if (iface->peek32(UE_REG_RB_MISC_TEST32) != file_hash){
        iface.reset();
        usrp_e100_load_fpga(usrp_e100_fpga_image);
        sleep(1); ///\todo do this better one day.
        UHD_MSG(status) << boost::format("re-Opening USRP-E on %s") % node << std::endl;
        iface = usrp_e100_iface::make(node);
    }

    //store the hash into the FPGA register
    iface->poke32(UE_REG_SR_MISC_TEST32, file_hash);

    //check that the hash can be readback correctly
    if (iface->peek32(UE_REG_RB_MISC_TEST32) != file_hash){
        UHD_MSG(error) << boost::format(
            "The FPGA hash readback failed!\n"
            "The FPGA is either clocked improperly\n"
            "or the FPGA build is not compatible.\n"
        );
    }

    //check that the compatibility is correct
    const boost::uint16_t fpga_compat_num = iface->peek16(UE_REG_MISC_COMPAT);
    if (fpga_compat_num != USRP_E_FPGA_COMPAT_NUM){
        throw uhd::runtime_error(str(boost::format(
            "\nPlease update the FPGA image for your device.\n"
            "See the application notes for USRP E-Series for instructions.\n"
            "Expected FPGA compatibility number 0x%x, but got 0x%x:\n"
            "The FPGA build is not compatible with the host code build."
        ) % USRP_E_FPGA_COMPAT_NUM % fpga_compat_num));
    }

    return device::sptr(new usrp_e100_impl(iface, device_addr));
}

UHD_STATIC_BLOCK(register_usrp_e100_device){
    device::register_device(&usrp_e100_find, &usrp_e100_make);
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp_e100_impl::usrp_e100_impl(
    usrp_e100_iface::sptr iface,
    const device_addr_t &device_addr
):
    _iface(iface),
    _data_xport(usrp_e100_make_mmap_zero_copy(_iface)),
    _recv_frame_size(std::min(_data_xport->get_recv_frame_size(), size_t(device_addr.cast<double>("recv_frame_size", 1e9)))),
    _send_frame_size(std::min(_data_xport->get_send_frame_size(), size_t(device_addr.cast<double>("send_frame_size", 1e9))))
{

    //setup interfaces into hardware
    const double master_clock_rate = device_addr.cast<double>("master_clock_rate", 64e6);
    _clock_ctrl = usrp_e100_clock_ctrl::make(_iface, master_clock_rate);
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

    //set default subdev specs
    this->mboard_set(MBOARD_PROP_RX_SUBDEV_SPEC, subdev_spec_t());
    this->mboard_set(MBOARD_PROP_TX_SUBDEV_SPEC, subdev_spec_t());

    //init the io send/recv
    io_init();

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

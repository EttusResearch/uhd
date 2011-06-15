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

#include "e100_impl.hpp"
#include "e100_regs.hpp"
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
static device_addrs_t e100_find(const device_addr_t &hint){
    device_addrs_t e100_addrs;

    //return an empty list of addresses when type is set to non-usrp-e
    if (hint.has_key("type") and hint["type"] != "e100") return e100_addrs;

    //device node not provided, assume its 0
    if (not hint.has_key("node")){
        device_addr_t new_addr = hint;
        new_addr["node"] = "/dev/usrp_e0";
        return e100_find(new_addr);
    }

    //use the given device node name
    if (fs::exists(hint["node"])){
        device_addr_t new_addr;
        new_addr["type"] = "e100";
        new_addr["node"] = fs::system_complete(fs::path(hint["node"])).string();
        try{
            e100_iface::sptr iface = e100_iface::make();
            new_addr["name"] = iface->mb_eeprom["name"];
            new_addr["serial"] = iface->mb_eeprom["serial"];
        }
        catch(const std::exception &e){
            new_addr["name"] = "";
            new_addr["serial"] = "";
        }
        if (
            (not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
            (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
        ){
            e100_addrs.push_back(new_addr);
        }
    }

    return e100_addrs;
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

static device::sptr e100_make(const device_addr_t &device_addr){

    //setup the main interface into fpga
    const std::string node = device_addr["node"];
    e100_iface::sptr iface = e100_iface::make();

    //extract the fpga path for usrp-e and compute hash
    const std::string e100_fpga_image = find_image_path(device_addr.get("fpga", E100_FPGA_FILE_NAME));
    const boost::uint32_t file_hash = boost::uint32_t(hash_fpga_file(e100_fpga_image));

    //When the hash does not match:
    // - close the device node
    // - load the fpga bin file
    // - re-open the device node
    iface->open(node); //open here so we can do FPGA hash check
    if (iface->peek32(E100_REG_RB_MISC_TEST32) != file_hash){
        iface->close();
        e100_load_fpga(e100_fpga_image);
        iface->open(node);
    }

    //setup clock control here to ensure that the FPGA has a good clock before we continue
    const double master_clock_rate = device_addr.cast<double>("master_clock_rate", E100_DEFAULT_CLOCK_RATE);
    e100_clock_ctrl::sptr clock_ctrl = e100_clock_ctrl::make(iface, master_clock_rate);

    //Perform wishbone readback tests, these tests also write the hash
    bool test_fail = false;
    UHD_MSG(status) << "Performing wishbone readback test... " << std::flush;
    for (size_t i = 0; i < 100; i++){
        iface->poke32(E100_REG_SR_MISC_TEST32, file_hash);
        test_fail = iface->peek32(E100_REG_RB_MISC_TEST32) != file_hash;
        if (test_fail) break; //exit loop on any failure
    }
    UHD_MSG(status) << ((test_fail)? " fail" : "pass") << std::endl;

    if (test_fail) UHD_MSG(error) << boost::format(
        "The FPGA is either clocked improperly\n"
        "or the FPGA build is not compatible.\n"
        "Subsequent errors may follow...\n"
    );

    //check that the compatibility is correct
    const boost::uint16_t fpga_compat_num = iface->peek16(E100_REG_MISC_COMPAT);
    if (fpga_compat_num != E100_FPGA_COMPAT_NUM){
        throw uhd::runtime_error(str(boost::format(
            "\nPlease update the FPGA image for your device.\n"
            "See the application notes for USRP E-Series for instructions.\n"
            "Expected FPGA compatibility number 0x%x, but got 0x%x:\n"
            "The FPGA build is not compatible with the host code build."
        ) % E100_FPGA_COMPAT_NUM % fpga_compat_num));
    }

    return device::sptr(new e100_impl(device_addr, iface, clock_ctrl));
}

UHD_STATIC_BLOCK(register_e100_device){
    device::register_device(&e100_find, &e100_make);
}

/***********************************************************************
 * Structors
 **********************************************************************/
e100_impl::e100_impl(
    const uhd::device_addr_t &device_addr,
    e100_iface::sptr iface,
    e100_clock_ctrl::sptr clock_ctrl
):
    _iface(iface),
    _clock_ctrl(clock_ctrl),
    _codec_ctrl(e100_codec_ctrl::make(_iface)),
    _data_xport(e100_make_mmap_zero_copy(_iface)),
    _recv_frame_size(std::min(_data_xport->get_recv_frame_size(), size_t(device_addr.cast<double>("recv_frame_size", 1e9)))),
    _send_frame_size(std::min(_data_xport->get_send_frame_size(), size_t(device_addr.cast<double>("send_frame_size", 1e9))))
{

    //setup otw types
    _send_otw_type.width = 16;
    _send_otw_type.shift = 0;
    _send_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    _recv_otw_type.width = 16;
    _recv_otw_type.shift = 0;
    _recv_otw_type.byteorder = otw_type_t::BO_LITTLE_ENDIAN;

    //initialize the mboard
    mboard_init();

    //initialize the dboards
    dboard_init();

    //initialize the dsps
    dsp_init();

    //init the codec properties
    codec_init();

    //set default subdev specs
    this->mboard_set(MBOARD_PROP_RX_SUBDEV_SPEC, subdev_spec_t());
    this->mboard_set(MBOARD_PROP_TX_SUBDEV_SPEC, subdev_spec_t());

    //init the io send/recv
    io_init();

}

e100_impl::~e100_impl(void){
    /* NOP */
}

/***********************************************************************
 * Device Get
 **********************************************************************/
void e100_impl::get(const wax::obj &key_, wax::obj &val){
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
void e100_impl::set(const wax::obj &, const wax::obj &){
    UHD_THROW_PROP_SET_ERROR();
}

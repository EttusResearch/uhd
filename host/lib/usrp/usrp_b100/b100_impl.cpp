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

#include "b100_impl.hpp"
#include "b100_ctrl.hpp"
#include "fpga_regs_standard.h"
#include "usrp_spi_defs.h"
#include <uhd/transport/usb_control.hpp>
#include "ctrl_packet.hpp"
#include <uhd/usrp/device_props.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/exception.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/images.hpp>
#include <uhd/utils/safe_call.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include "b100_regs.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

const boost::uint16_t B100_VENDOR_ID  = 0x2500;
const boost::uint16_t B100_PRODUCT_ID = 0x0001;
const boost::uint16_t FX2_VENDOR_ID    = 0x04b4;
const boost::uint16_t FX2_PRODUCT_ID   = 0x8613;

/***********************************************************************
 * Discovery
 **********************************************************************/
static device_addrs_t b100_find(const device_addr_t &hint)
{
    device_addrs_t b100_addrs;

    //return an empty list of addresses when type is set to non-b100
    if (hint.has_key("type") and hint["type"] != "b100") return b100_addrs;

    //extract the firmware path for the b100
    std::string b100_fw_image;
    try{
        b100_fw_image = find_image_path(
            hint.has_key("fw")? hint["fw"] : "usrp_b100_fw_c2.ihx"
        );
    }
    catch(...){
        UHD_MSG(warning) << boost::format(
            "Could not locate B100 firmware.\n"
            "Please install the images package.\n"
        );
        return b100_addrs;
    }

    boost::uint16_t vid = hint.has_key("uninit") ? FX2_VENDOR_ID : B100_VENDOR_ID;
    boost::uint16_t pid = hint.has_key("uninit") ? FX2_PRODUCT_ID : B100_PRODUCT_ID;

    // Important note:
    // The get device list calls are nested inside the for loop.
    // This allows the usb guts to decontruct when not in use,
    // so that re-enumeration after fw load can occur successfully.
    // This requirement is a courtesy of libusb1.0 on windows.

    //find the usrps and load firmware
    BOOST_FOREACH(usb_device_handle::sptr handle, usb_device_handle::get_device_list(vid, pid)) {
        try {
            fx2_ctrl::make(usb_control::make(handle))->usrp_load_firmware(b100_fw_image);
        } catch (...) {
            UHD_MSG(status) << "Interface claimed, ignoring device" << std::endl;
        }
    }

    //get descriptors again with serial number, but using the initialized VID/PID now since we have firmware
    vid = B100_VENDOR_ID;
    pid = B100_PRODUCT_ID;

    BOOST_FOREACH(usb_device_handle::sptr handle, usb_device_handle::get_device_list(vid, pid)) {
        device_addr_t new_addr;
        new_addr["type"] = "b100";

        //Attempt to read the name from the EEPROM and perform filtering.
        //This operation can throw due to compatibility mismatch.
        try{
            usb_control::sptr control = usb_control::make(handle);
            b100_iface::sptr iface = b100_iface::make(fx2_ctrl::make(control));
            new_addr["name"] = iface->mb_eeprom["name"];
            new_addr["serial"] = handle->get_serial();
        }
        catch(const uhd::exception &){
            //set these values as empty string so the device may still be found
            //and the filter's below can still operate on the discovered device
            new_addr["name"] = "";
            new_addr["serial"] = "";
        }

        //this is a found b100 when the hint serial and name match or blank
        if (
            (not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
            (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
        ){
            b100_addrs.push_back(new_addr);
        }
    }

    return b100_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr b100_make(const device_addr_t &device_addr){

    //extract the FPGA path for the B100
    std::string b100_fpga_image = find_image_path(
        device_addr.has_key("fpga")? device_addr["fpga"] : "usrp_b100_fpga_c3.bin"
    );

    //try to match the given device address with something on the USB bus
    std::vector<usb_device_handle::sptr> device_list =
        usb_device_handle::get_device_list(B100_VENDOR_ID, B100_PRODUCT_ID);

    //locate the matching handle in the device list
    usb_device_handle::sptr handle;
    BOOST_FOREACH(usb_device_handle::sptr dev_handle, device_list) {
        if (dev_handle->get_serial() == device_addr["serial"]){
            handle = dev_handle;
            break;
        }
    }
    UHD_ASSERT_THROW(handle.get() != NULL); //better be found

    //create control objects and a data transport
    usb_control::sptr fx2_transport = usb_control::make(handle);
    fx2_ctrl::sptr fx2_ctrl = fx2_ctrl::make(fx2_transport);
    fx2_ctrl->usrp_load_fpga(b100_fpga_image);

    device_addr_t data_xport_args;
    data_xport_args["recv_frame_size"] = device_addr.get("recv_frame_size", "16384");
    data_xport_args["num_recv_frames"] = device_addr.get("num_recv_frames", "16");
    data_xport_args["send_frame_size"] = device_addr.get("send_frame_size", "16384");
    data_xport_args["num_send_frames"] = device_addr.get("num_send_frames", "16");

    usb_zero_copy::sptr data_transport = usb_zero_copy::make_wrapper(
        usb_zero_copy::make(
            handle,        // identifier
            6,             // IN endpoint
            2,             // OUT endpoint
            data_xport_args    // param hints
        )
    );

    //create the control transport
    device_addr_t ctrl_xport_args;
    ctrl_xport_args["recv_frame_size"] = boost::lexical_cast<std::string>(CTRL_PACKET_LENGTH);
    ctrl_xport_args["num_recv_frames"] = "16";
    ctrl_xport_args["send_frame_size"] = boost::lexical_cast<std::string>(CTRL_PACKET_LENGTH);
    ctrl_xport_args["num_send_frames"] = "4";

    usb_zero_copy::sptr ctrl_transport = usb_zero_copy::make(
        handle,
        8,
        4,
        ctrl_xport_args
    );

    const double master_clock_rate = device_addr.cast<double>("master_clock_rate", 64e6);


    //create the b100 implementation guts
    return device::sptr(new b100_impl(data_transport, ctrl_transport, fx2_ctrl, master_clock_rate));
}

UHD_STATIC_BLOCK(register_b100_device){
    device::register_device(&b100_find, &b100_make);
}

/***********************************************************************
 * Structors
 **********************************************************************/
b100_impl::b100_impl(uhd::transport::usb_zero_copy::sptr data_transport,
                         uhd::transport::usb_zero_copy::sptr ctrl_transport,
                         uhd::usrp::fx2_ctrl::sptr fx2_ctrl,
                         const double master_clock_rate)
 : _data_transport(data_transport), _fx2_ctrl(fx2_ctrl)
{
    //this is the handler object for FPGA control packets
    _fpga_ctrl = b100_ctrl::make(ctrl_transport);

    _iface = b100_iface::make(_fx2_ctrl, _fpga_ctrl);

    //create clock interface
    _clock_ctrl = b100_clock_ctrl::make(_iface, master_clock_rate);

    //create codec interface
    _codec_ctrl = b100_codec_ctrl::make(_iface);

    //initialize the codecs
    codec_init();

    //initialize the mboard
    mboard_init();

    //initialize the dboards
    dboard_init();

    //initialize the dsps
    rx_ddc_init();

    //initialize the dsps
    tx_duc_init();

    //init the subdev specs
    this->mboard_set(MBOARD_PROP_RX_SUBDEV_SPEC, subdev_spec_t());
    this->mboard_set(MBOARD_PROP_TX_SUBDEV_SPEC, subdev_spec_t());

    //initialize the send/recv buffs
    io_init();
}

b100_impl::~b100_impl(void){
    /* NOP */
}

bool b100_impl::recv_async_msg(uhd::async_metadata_t &md, double timeout){
    return _fpga_ctrl->recv_async_msg(md, timeout);
}

/***********************************************************************
 * Device Get
 **********************************************************************/
void b100_impl::get(const wax::obj &key_, wax::obj &val)
{
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<device_prop_t>()){
    case DEVICE_PROP_NAME:
        val = std::string("USRP-B100 device");
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
void b100_impl::set(const wax::obj &, const wax::obj &)
{
    UHD_THROW_PROP_SET_ERROR();
}

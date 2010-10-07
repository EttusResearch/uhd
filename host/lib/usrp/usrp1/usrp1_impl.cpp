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

#include "usrp1_impl.hpp"
#include "usrp1_ctrl.hpp"
#include "fpga_regs_standard.h"
#include "usrp_spi_defs.h"
#include <uhd/transport/usb_control.hpp>
#include <uhd/usrp/device_props.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/utils/warning.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/images.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

const boost::uint16_t USRP1_VENDOR_ID  = 0xfffe;
const boost::uint16_t USRP1_PRODUCT_ID = 0x0002;
const boost::uint16_t FX2_VENDOR_ID    = 0x04b4;
const boost::uint16_t FX2_PRODUCT_ID   = 0x8613;

const std::vector<usrp1_impl::dboard_slot_t> usrp1_impl::_dboard_slots = boost::assign::list_of
    (usrp1_impl::DBOARD_SLOT_A)(usrp1_impl::DBOARD_SLOT_B)
;

/***********************************************************************
 * Discovery
 **********************************************************************/
static device_addrs_t usrp1_find(const device_addr_t &hint)
{
    device_addrs_t usrp1_addrs;

    //return an empty list of addresses when type is set to non-usrp1
    if (hint.has_key("type") and hint["type"] != "usrp1") return usrp1_addrs;

    //extract the firmware path for the USRP1
    std::string usrp1_fw_image;
    try{
        usrp1_fw_image = find_image_path(
            hint.has_key("fw")? hint["fw"] : "usrp1_fw.ihx"
        );
    }
    catch(...){
        uhd::print_warning(
            "Could not locate USRP1 firmware.\n"
            "Please install the images package.\n"
        );
        return usrp1_addrs;
    }
    //std::cout << "USRP1 firmware image: " << usrp1_fw_image << std::endl;

    boost::uint16_t vid = hint.has_key("uninit") ? FX2_VENDOR_ID : USRP1_VENDOR_ID;
    boost::uint16_t pid = hint.has_key("uninit") ? FX2_PRODUCT_ID : USRP1_PRODUCT_ID;

    // Important note:
    // The get device list calls are nested inside the for loop.
    // This allows the usb guts to decontruct when not in use,
    // so that re-enumeration after fw load can occur successfully.
    // This requirement is a courtesy of libusb1.0 on windows.

    //find the usrps and load firmware
    BOOST_FOREACH(usb_device_handle::sptr handle, usb_device_handle::get_device_list(vid, pid)) {
        usrp_ctrl::make(usb_control::make(handle))->usrp_load_firmware(usrp1_fw_image);
    }

    //get descriptors again with serial number, but using the initialized VID/PID now since we have firmware
    vid = USRP1_VENDOR_ID;
    pid = USRP1_PRODUCT_ID;

    BOOST_FOREACH(usb_device_handle::sptr handle, usb_device_handle::get_device_list(vid, pid)) {
        device_addr_t new_addr;
        new_addr["type"] = "usrp1";
        new_addr["serial"] = handle->get_serial();
        //this is a found usrp1 when a hint serial is not specified or it matches
        if (not hint.has_key("serial") or hint["serial"] == new_addr["serial"]){
            usrp1_addrs.push_back(new_addr);
        }
    }

    return usrp1_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr usrp1_make(const device_addr_t &device_addr){

    //extract the FPGA path for the USRP1
    std::string usrp1_fpga_image = find_image_path(
        device_addr.has_key("fpga")? device_addr["fpga"] : "usrp1_fpga.rbf"
    );
    //std::cout << "USRP1 FPGA image: " << usrp1_fpga_image << std::endl;

    //try to match the given device address with something on the USB bus
    std::vector<usb_device_handle::sptr> device_list =
        usb_device_handle::get_device_list(USRP1_VENDOR_ID, USRP1_PRODUCT_ID);

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
    usb_control::sptr ctrl_transport = usb_control::make(handle);
    usrp_ctrl::sptr usrp_ctrl = usrp_ctrl::make(ctrl_transport);
    usrp_ctrl->usrp_load_fpga(usrp1_fpga_image);
    usb_zero_copy::sptr data_transport = usb_zero_copy::make(
        handle,        // identifier
        6,             // IN endpoint
        2,             // OUT endpoint
        device_addr    // param hints
    );

    //create the usrp1 implementation guts
    return device::sptr(new usrp1_impl(data_transport, usrp_ctrl));
}

UHD_STATIC_BLOCK(register_usrp1_device){
    device::register_device(&usrp1_find, &usrp1_make);
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp1_impl::usrp1_impl(uhd::transport::usb_zero_copy::sptr data_transport,
                       usrp_ctrl::sptr ctrl_transport)
 : _data_transport(data_transport), _ctrl_transport(ctrl_transport)
{
    _iface = usrp1_iface::make(ctrl_transport);

    //create clock interface
    _clock_ctrl = usrp1_clock_ctrl::make(_iface);

    //create codec interface
    _codec_ctrls[DBOARD_SLOT_A] = usrp1_codec_ctrl::make(
        _iface, _clock_ctrl, SPI_ENABLE_CODEC_A
    );
    _codec_ctrls[DBOARD_SLOT_B] = usrp1_codec_ctrl::make(
        _iface, _clock_ctrl, SPI_ENABLE_CODEC_B
    );

    //initialize the codecs
    codec_init();

    //initialize the mboard
    mboard_init();

    //initialize the dboards
    dboard_init();

    //initialize the dsps
    rx_dsp_init();

    //initialize the dsps
    tx_dsp_init();

    //initialize the send/recv
    io_init();

    //turn on the transmitter
    _ctrl_transport->usrp_tx_enable(true);

    //init the subdev specs
    this->mboard_set(MBOARD_PROP_RX_SUBDEV_SPEC, subdev_spec_t());
    this->mboard_set(MBOARD_PROP_TX_SUBDEV_SPEC, subdev_spec_t());
}

usrp1_impl::~usrp1_impl(void){
    /* NOP */
}

bool usrp1_impl::recv_async_msg(uhd::async_metadata_t &, double timeout){
    //dummy fill-in for the recv_async_msg
    boost::this_thread::sleep(boost::posix_time::microseconds(long(timeout*1e6)));
    return false;
}

/***********************************************************************
 * Device Get
 **********************************************************************/
void usrp1_impl::get(const wax::obj &key_, wax::obj &val)
{
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<device_prop_t>()){
    case DEVICE_PROP_NAME:
        val = std::string("usrp1 device");
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
void usrp1_impl::set(const wax::obj &, const wax::obj &)
{
    UHD_THROW_PROP_SET_ERROR();
}

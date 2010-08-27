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
#include <uhd/utils/assert.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/images.hpp>
#include <boost/format.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

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
    std::string usrp1_fw_image = find_image_path(
        hint.has_key("fw")? hint["fw"] : "usrp1_fw.ihx"
    );
    std::cout << "USRP1 firmware image: " << usrp1_fw_image << std::endl;

    //see what we got on the USB bus
    std::vector<usb_device_handle::sptr> device_list =
        usb_device_handle::get_device_list();

    //find the usrps and load firmware
    BOOST_FOREACH(usb_device_handle::sptr handle, device_list) {
        if (handle->get_vendor_id() == 0xfffe &&
            handle->get_product_id() == 0x0002) {

            usb_control::sptr ctrl_transport = usb_control::make(handle);
            usrp_ctrl::sptr usrp_ctrl = usrp_ctrl::make(ctrl_transport);
            usrp_ctrl->usrp_load_firmware(usrp1_fw_image);
        }
    }

    //wait for things to settle
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));

    //get descriptors again with serial number
    device_list = usb_device_handle::get_device_list();

    BOOST_FOREACH(usb_device_handle::sptr handle, device_list) {
        if (handle->get_vendor_id() == 0xfffe &&
            handle->get_product_id() == 0x0002) {

            device_addr_t new_addr;
            new_addr["type"] = "usrp1";
            new_addr["serial"] = handle->get_serial();
            usrp1_addrs.push_back(new_addr);
        }
    }

    return usrp1_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr usrp1_make(const device_addr_t &device_addr)
{
    //extract the FPGA path for the USRP1
    std::string usrp1_fpga_image = find_image_path(
        device_addr.has_key("fpga")? device_addr["fpga"] : "usrp1_fpga.rbf"
    );
    std::cout << "USRP1 FPGA image: " << usrp1_fpga_image << std::endl;

    //try to match the given device address with something on the USB bus
    std::vector<usb_device_handle::sptr> device_list =
        usb_device_handle::get_device_list();

    //create data and control transports
    usb_zero_copy::sptr data_transport;
    usrp_ctrl::sptr usrp_ctrl;

    BOOST_FOREACH(usb_device_handle::sptr handle, device_list) {
        if (handle->get_vendor_id() == 0xfffe &&
            handle->get_product_id() == 0x0002 &&
            handle->get_serial() == device_addr["serial"]) {

            usb_control::sptr ctrl_transport = usb_control::make(handle);
            usrp_ctrl = usrp_ctrl::make(ctrl_transport);
            usrp_ctrl->usrp_load_fpga(usrp1_fpga_image);

            data_transport = usb_zero_copy::make(handle,        // identifier
                                                 6,             // IN endpoint
                                                 2,             // OUT endpoint
                                                 2 * (1 << 20), // buffer size
                                                 16384);        // transfer size
            break;
        }
    }

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
}

usrp1_impl::~usrp1_impl(void){
    /* NOP */
}

bool usrp1_impl::recv_async_msg(uhd::async_metadata_t &, size_t timeout_ms){
    //dummy fill-in for the recv_async_msg
    boost::this_thread::sleep(boost::posix_time::milliseconds(timeout_ms));
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

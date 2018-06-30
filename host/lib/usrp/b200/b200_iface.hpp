//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_B200_IFACE_HPP
#define INCLUDED_B200_IFACE_HPP

#include <uhd/transport/usb_control.hpp>
#include <uhd/types/serial.hpp> //i2c iface
#include <uhd/types/dict.hpp>
#include <uhdlib/usrp/common/ad9361_ctrl.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <stdint.h>

enum b200_product_t {
    B200,
    B210,
    B200MINI,
    B205MINI
};

// These are actual USB PIDs (not Ettus Product IDs)
const static uint16_t B200_VENDOR_ID         = 0x2500;
const static uint16_t B200_VENDOR_NI_ID      = 0x3923;
const static uint16_t B200_PRODUCT_ID        = 0x0020;
const static uint16_t B200MINI_PRODUCT_ID    = 0x0021;
const static uint16_t B205MINI_PRODUCT_ID    = 0x0022;
const static uint16_t B200_PRODUCT_NI_ID     = 0x7813;
const static uint16_t B210_PRODUCT_NI_ID     = 0x7814;
const static uint16_t FX3_VID                = 0x04b4;
const static uint16_t FX3_DEFAULT_PID        = 0x00f3;
const static uint16_t FX3_REENUM_PID         = 0x00f0;

//! Map the USB PID to the product (only for PIDs that map to a single product)
static const uhd::dict<uint16_t, b200_product_t> B2XX_PID_TO_PRODUCT = boost::assign::map_list_of
        (B200_PRODUCT_NI_ID,    B200)
        (B210_PRODUCT_NI_ID,    B210)
        (B200MINI_PRODUCT_ID,   B200MINI)
        (B205MINI_PRODUCT_ID,   B205MINI)
;

static const std::string     B200_FW_FILE_NAME = "usrp_b200_fw.hex";

//! Map the EEPROM product ID codes to the product
static const uhd::dict<uint16_t, b200_product_t> B2XX_PRODUCT_ID = boost::assign::map_list_of
        (0x0001,             B200)
        (0x7737,             B200)
        (B200_PRODUCT_NI_ID, B200)
        (0x0002,             B210)
        (0x7738,             B210)
        (B210_PRODUCT_NI_ID, B210)
        (0x0003,             B200MINI)
        (0x7739,             B200MINI)
        (0x0004,             B205MINI)
        (0x773a,             B205MINI)
;


static const uhd::dict<b200_product_t, std::string> B2XX_STR_NAMES = boost::assign::map_list_of
        (B200,      "B200")
        (B210,      "B210")
        (B200MINI,  "B200mini")
        (B205MINI,  "B205mini")
;

static const uhd::dict<b200_product_t, std::string> B2XX_FPGA_FILE_NAME = boost::assign::map_list_of
        (B200, "usrp_b200_fpga.bin")
        (B210, "usrp_b210_fpga.bin")
        (B200MINI, "usrp_b200mini_fpga.bin")
        (B205MINI, "usrp_b205mini_fpga.bin")
;


class UHD_API b200_iface: boost::noncopyable, public virtual uhd::i2c_iface {
public:
    typedef boost::shared_ptr<b200_iface> sptr;

    /*!
     * Make a b200 interface object from a control transport
     * \param usb_ctrl a USB control transport
     * \return a new b200 interface object
     */
    static sptr make(uhd::transport::usb_control::sptr usb_ctrl);

    //! query the device USB speed (2, 3)
    virtual uint8_t get_usb_speed(void) = 0;

    //! get the current status of the FX3
    virtual uint8_t get_fx3_status(void) = 0;

    //! get the current status of the FX3
    virtual uint16_t get_compat_num(void) = 0;

    //! load a firmware image
    virtual void load_firmware(const std::string filestring, bool force=false) = 0;

    //! reset the FX3
    virtual void reset_fx3(void) = 0;

    //! reset the GPIF state machine
    virtual void reset_gpif(void) = 0;

    //! set the FPGA_RESET line
    virtual void set_fpga_reset_pin(const bool reset) = 0;

    //! load an FPGA image
    virtual uint32_t load_fpga(const std::string filestring, bool force=false) = 0;

    virtual void write_eeprom(uint16_t addr, uint16_t offset, const uhd::byte_vector_t &bytes) = 0;

    virtual uhd::byte_vector_t read_eeprom(uint16_t addr, uint16_t offset, size_t num_bytes) = 0;

    static std::string fx3_state_string(uint8_t state);
};


#endif /* INCLUDED_B200_IFACE_HPP */

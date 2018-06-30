//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/usb_device_handle.hpp>
#include <uhd/transport/usb_control.hpp>
#include <uhd/transport/usb_zero_copy.hpp>
#include <uhd/exception.hpp>

using namespace uhd;
using namespace uhd::transport;

usb_control::~usb_control(void){
    /* NOP */
}

std::vector<usb_device_handle::sptr> usb_device_handle::get_device_list(uint16_t, uint16_t){
    return std::vector<usb_device_handle::sptr>(); //empty list
}

usb_control::sptr usb_control::make(
        usb_device_handle::sptr,
        const int
) {
    throw uhd::not_implemented_error("no usb support -> usb_control::make not implemented");
}

usb_zero_copy::sptr usb_zero_copy::make(
    usb_device_handle::sptr,
    const int,
    const unsigned char,
    const int,
    const unsigned char,
    const device_addr_t &
){
    throw uhd::not_implemented_error("no usb support -> usb_zero_copy::make not implemented");
}

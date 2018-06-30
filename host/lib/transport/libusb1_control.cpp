//
// Copyright 2010,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "libusb1_base.hpp"
#include <uhd/transport/usb_control.hpp>
#include <boost/thread/mutex.hpp>

using namespace uhd::transport;

usb_control::~usb_control(void){
    /* NOP */
}

/***********************************************************************
 * libusb-1.0 implementation of USB control transport
 **********************************************************************/
class libusb_control_impl : public usb_control {
public:
    libusb_control_impl(libusb::device_handle::sptr handle, const int interface):
        _handle(handle)
    {
        _handle->claim_interface(interface);
    }

    virtual ~libusb_control_impl(void);

    int submit(uint8_t request_type,
               uint8_t request,
               uint16_t value,
               uint16_t index,
               unsigned char *buff,
               uint16_t length,
               uint32_t libusb_timeout = 0
    ){
        boost::mutex::scoped_lock lock(_mutex);
        return libusb_control_transfer(_handle->get(),
                                       request_type,
                                       request,
                                       value,
                                       index,
                                       buff,
                                       length,
                                       libusb_timeout);
    }

private:
    libusb::device_handle::sptr _handle;
    boost::mutex _mutex;
};

libusb_control_impl::~libusb_control_impl(void)  {
  /* NOP */
}

/***********************************************************************
 * USB control public make functions
 **********************************************************************/
usb_control::sptr usb_control::make(usb_device_handle::sptr handle, const int interface){
    return sptr(new libusb_control_impl(libusb::device_handle::get_cached_handle(
        boost::static_pointer_cast<libusb::special_handle>(handle)->get_device()
    ), interface));
}

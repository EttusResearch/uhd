//
// Copyright 2010-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_USB_CONTROL_HPP
#define INCLUDED_UHD_TRANSPORT_USB_CONTROL_HPP

#include <uhd/transport/usb_device_handle.hpp>

namespace uhd { namespace transport {

class UHD_API usb_control : boost::noncopyable {
public:
    typedef boost::shared_ptr<usb_control> sptr;

    virtual ~usb_control(void);

    /*!
     * Create a new USB control transport:
     * This transport is for sending and receiving control information from
     * the host to device using the Default Control Pipe.
     *
     * \param handle a device handle that uniquely identifies a USB device
     * \param interface the USB interface number for the control transport
     */
    static sptr make(usb_device_handle::sptr handle, const int interface);

    /*!
     * Submit a USB device request:
     * Blocks until the request returns
     *
     * For format and corresponding USB request fields
     * see USB Specification Revision 2.0 - 9.3 USB Device Requests
     *
     * Usage is device specific
     *
     * \param request_type 1-byte bitmask (bmRequestType)
     * \param request      1-byte (bRequest)
     * \param value        2-byte (wValue)
     * \param index        2-byte (wIndex)
     * \param buff         buffer to hold send or receive data
     * \param length       2-byte (wLength)
     * \param timeout      4-byte (timeout, default is infinite wait)
     * \return             number of bytes submitted or error code
     */
    virtual int submit(uint8_t request_type,
                       uint8_t request,
                       uint16_t value,
                       uint16_t index, 
                       unsigned char *buff,
                       uint16_t length,
                       uint32_t timeout = 0) = 0;
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_USB_CONTROL_HPP */

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

#ifndef INCLUDED_UHD_TRANSPORT_USB_CONTROL_HPP
#define INCLUDED_UHD_TRANSPORT_USB_CONTROL_HPP

#include <uhd/transport/usb_device_handle.hpp>

namespace uhd { namespace transport {

class UHD_API usb_control : boost::noncopyable {
public:
    typedef boost::shared_ptr<usb_control> sptr;

    /*!
     * Create a new usb control transport:
     * This transport is for sending and receiving control information from
     * the host to device using the Default Control Pipe.
     *
     * \param handle a device handle that uniquely identifies a USB device
     * \param interface the USB interface number for the control transport
     */
    static sptr make(usb_device_handle::sptr handle, const size_t interface);

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
     * \return             number of bytes submitted or error code
     */
    virtual ssize_t submit(boost::uint8_t request_type,
                          boost::uint8_t request,
                          boost::uint16_t value,
                          boost::uint16_t index, 
                          unsigned char *buff,
                          boost::uint16_t length) = 0; 
};

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_USB_CONTROL_HPP */

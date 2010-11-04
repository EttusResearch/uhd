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

#ifndef INCLUDED_UHD_TRANSPORT_USB_DEVICE_HANDLE_HPP
#define INCLUDED_UHD_TRANSPORT_USB_DEVICE_HANDLE_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include <vector>

namespace uhd { namespace transport {

/*!
 * Device handle class that represents a USB device
 * Used for identifying devices on the USB bus and selecting which device is
 * used when creating a  USB transport. A minimal subset of USB descriptor
 * fields are used. Fields can be found in the USB 2.0 specification Table
 * 9-8 (Standard Device Descriptor). In addition to fields of the device
 * descriptor, the interface returns the device's USB device address.
 *
 * Note: The USB 2.0 Standard Device Descriptor contains an index rather then
 *       a true descriptor serial number string. This interface returns the
 *       actual string descriptor.
 */
class UHD_API usb_device_handle : boost::noncopyable {
public:
    typedef boost::shared_ptr<usb_device_handle> sptr;

    /*!
     * Return the device's serial number 
     * \return a string describing the device's serial number
     */
    virtual std::string get_serial() const = 0;

    /*!
     * Return the device's Vendor ID (usually assigned by the USB-IF)
     * \return a Vendor ID
     */
    virtual boost::uint16_t get_vendor_id() const = 0;

    /*!
     * Return the device's Product ID (usually assigned by manufacturer)
     * \return a Product ID
     */
    virtual boost::uint16_t get_product_id() const = 0;

    /*!
     * Return a vector of USB devices on this host 
     * \return a vector of USB device handles that match vid and pid
     */
    static std::vector<usb_device_handle::sptr> get_device_list(boost::uint16_t vid, boost::uint16_t pid);

}; //namespace usb

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_USB_DEVICE_HANDLE_HPP */

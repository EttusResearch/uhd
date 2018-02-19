//
// Copyright 2010-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TRANSPORT_USB_DEVICE_HANDLE_HPP
#define INCLUDED_UHD_TRANSPORT_USB_DEVICE_HANDLE_HPP

#include <uhd/config.hpp>
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <stdint.h>
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
    typedef std::pair<uint16_t, uint16_t> vid_pid_pair_t;

    virtual ~usb_device_handle(void);

    /*!
     * Return the device's serial number
     * \return a string describing the device's serial number
     */
    virtual std::string get_serial() const = 0;

    /*!
     * Return the device's manufacturer identification string
     * \return a string describing the device's manufacturer string
     */
    virtual std::string get_manufacturer() const = 0;

    /*!
     * Return the device's product identification string
     * \return a string describing the device's product string
     */
    virtual std::string get_product() const = 0;

    /*!
     * Return the device's Vendor ID (usually assigned by the USB-IF)
     * \return a Vendor ID
     */
    virtual uint16_t get_vendor_id() const = 0;

    /*!
     * Return the device's Product ID (usually assigned by manufacturer)
     * \return a Product ID
     */
    virtual uint16_t get_product_id() const = 0;

    /*!
     * Test whether the firmware is loaded on the device.
     * \return true if firmware is loaded
     */
    virtual bool firmware_loaded() = 0;

    /*!
     * Return a vector of USB devices on this host
     * \return a vector of USB device handles that match vid and pid
     */
    static std::vector<usb_device_handle::sptr> get_device_list(uint16_t vid, uint16_t pid);
    static std::vector<usb_device_handle::sptr> get_device_list(const std::vector<usb_device_handle::vid_pid_pair_t>& vid_pid_pair_list);


}; //namespace usb

}} //namespace

#endif /* INCLUDED_UHD_TRANSPORT_USB_DEVICE_HANDLE_HPP */

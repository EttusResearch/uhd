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

#ifndef INCLUDED_TRANSPORT_LIBUSB_HPP
#define INCLUDED_TRANSPORT_LIBUSB_HPP

#include <uhd/config.hpp>
#include <uhd/transport/usb_device_handle.hpp>
#include <libusb-1.0/libusb.h>

namespace uhd { namespace transport {

namespace libusb {
    /*
     * Initialize libusb and set debug level
     * Takes a pointer to context pointer because that's
     * how libusb rolls. Debug levels.
     *     
     *   Level 0: no messages ever printed by the library (default)
     *   Level 1: error messages are printed to stderr
     *   Level 2: warning and error messages are printed to stderr
     *   Level 3: informational messages are printed to stdout, warning
     *            and error messages are printed to stderr
     *
     * \param ctx pointer to context pointer
     * \param debug_level
     */
    void init(libusb_context **ctx, int debug_level);

    /*
     * Get a list of Free Software Foundation devices (Vendor ID 0xfffe)
     * As opposed to the public USB device handle interface, which returns
     * generic identifiers, this call returns device pointers speficic
     * to libusb.
     * \param ctx the libusb context used for init
     * \return a vector of libusb devices
     */
    std::vector<libusb_device *> get_fsf_device_list(libusb_context *ctx);

    /*
     * Open the device specified by a generic handle
     * Find the libusb_device cooresponding to the generic handle
     * and open it for I/O, which returns a libusb_device_handle
     * ready for an interface
     * \param ctx the libusb context used for init
     * \return a libusb_device_handle ready for action 
     */
    libusb_device_handle *open_device(libusb_context *ctx,
                                      usb_device_handle::sptr handle);

    /*
     * Compare a libusb device with a generic handle 
     * Check the descriptors and open the device to check the
     * serial number string. Compare values against the given
     * handle. The libusb context is already implied in the
     * libusb_device.
     * \param dev a libusb_device pointer
     * \param handle a generic handle specifier
     * \return true if handle and device match, false otherwise
     */
    bool compare_device(libusb_device *dev, usb_device_handle::sptr handle);

    /*
     * Open an interface to the device
     * This is a logical operation for operating system housekeeping as
     * nothing is sent over the bus. The interface much correspond
     * to the USB device descriptors.
     * \param dev_handle libusb handle to an opened device
     * \param interface integer of the interface to use
     * \return true on success, false on error
     */
    bool open_interface(libusb_device_handle *dev_handle, int interface);

    /*
     * Get serial number 
     * The standard USB device descriptor contains an index to an
     * actual serial number string descriptor. The index is readily
     * readble, but the string descriptor requires probing the device.
     * Because this call attempts to open the device, it may not
     * succeed because not all USB devices are readily opened.
     * The default language is used for the request (English).
     * \param dev a libusb_device pointer
     * \return string serial number or 0 on error or unavailablity
     */
    std::string get_serial(libusb_device *dev);
}

}} //namespace

#endif /* INCLUDED_TRANSPORT_LIBUSB_HPP */

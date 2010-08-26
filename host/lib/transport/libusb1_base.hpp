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
    void init(libusb_context **ctx, int debug_level);

    libusb_device_handle *open_device(libusb_context *ctx,
                                      usb_device_handle::sptr handle);

    bool compare_device(libusb_device *dev, usb_device_handle::sptr handle);

    bool open_interface(libusb_device_handle *dev_handle, int interface);

    std::string get_serial(libusb_device *dev);
}

}} //namespace

#endif /* INCLUDED_TRANSPORT_LIBUSB_HPP */

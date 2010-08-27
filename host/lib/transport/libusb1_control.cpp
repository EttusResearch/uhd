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

#include "libusb1_base.hpp"
#include <uhd/transport/usb_control.hpp>

using namespace uhd::transport;

const int libusb_debug_level = 0;
const int libusb_timeout = 0;

/***********************************************************************
 * libusb-1.0 implementation of USB control transport
 **********************************************************************/
class libusb_control_impl : public usb_control {
public:
    libusb_control_impl(usb_device_handle::sptr handle);
    ~libusb_control_impl();

    size_t submit(boost::uint8_t request_type,
                  boost::uint8_t request,
                  boost::uint16_t value,
                  boost::uint16_t index,
                  unsigned char *buff,
                  boost::uint16_t length); 

private:
    libusb_context       *_ctx;
    libusb_device_handle *_dev_handle;
};


libusb_control_impl::libusb_control_impl(usb_device_handle::sptr handle)
{
    libusb::init(&_ctx, libusb_debug_level);

    _dev_handle = libusb::open_device(_ctx, handle);

    libusb::open_interface(_dev_handle, 0);
}


libusb_control_impl::~libusb_control_impl()
{
    libusb_close(_dev_handle);
    libusb_exit(_ctx);
}


size_t libusb_control_impl::submit(boost::uint8_t request_type,
                                   boost::uint8_t request,
                                   boost::uint16_t value,
                                   boost::uint16_t index, 
                                   unsigned char *buff,
                                   boost::uint16_t length) 
{
    return libusb_control_transfer(_dev_handle,
                                   request_type,
                                   request,
                                   value,
                                   index,
                                   buff, 
                                   length, 
                                   libusb_timeout);
}


/***********************************************************************
 * USB control public make functions
 **********************************************************************/
usb_control::sptr usb_control::make(usb_device_handle::sptr handle)
{
    return sptr(new libusb_control_impl(handle));
}

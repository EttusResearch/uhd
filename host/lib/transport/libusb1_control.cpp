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

#include <uhd/types/usb_descriptor.hpp>
#include <uhd/transport/usb_control.hpp>
#include <libusb-1.0/libusb.h>
#include <boost/asio.hpp>
#include <stdexcept>
#include <iostream>

using namespace uhd::transport;

static int libusb_debug_level = 0;
static int libusb_timeout = 0;

/***********************************************************************
 * libusb-1.0 implementation of USB control transport
 **********************************************************************/
class libusb_control_impl : public usb_control {
public:
    libusb_control_impl(uhd::usb_descriptor_t descriptor);
    ~libusb_control_impl();

    size_t submit(boost::uint8_t request_type,
                  boost::uint8_t request,
                  boost::uint16_t value,
                  boost::uint16_t index,
                  unsigned char *buff,
                  boost::uint16_t length); 

    static uhd::usb_descriptor_t create_descriptor(libusb_device *dev);
    static std::string get_serial(libusb_device *dev);

private:
    uhd::usb_descriptor_t _descriptor;

    libusb_context       *_ctx;
    libusb_device_handle *_dev_handle;

    bool open_device();
    bool open_interface();
    bool compare_device(libusb_device *dev, uhd::usb_descriptor_t descriptor);
};


libusb_control_impl::libusb_control_impl(uhd::usb_descriptor_t descriptor)
 :  _descriptor(descriptor), _ctx(NULL), _dev_handle(NULL)
{
    if (libusb_init(&_ctx) < 0)
        throw std::runtime_error("USB: failed to initialize libusb");

    libusb_set_debug(_ctx, libusb_debug_level);

    if (!open_device())
        throw std::runtime_error("USB: failed to open device");

    if (!open_interface())
        throw std::runtime_error("USB: failed to open device interface");
}


libusb_control_impl::~libusb_control_impl()
{
    libusb_close(_dev_handle);
    libusb_exit(_ctx);
}


uhd::usb_descriptor_t libusb_control_impl::create_descriptor(libusb_device *dev)
{
    libusb_device_descriptor desc;

    if (libusb_get_device_descriptor(dev, &desc) < 0)
        throw std::runtime_error("USB: failed to get device descriptor");

    uhd::usb_descriptor_t descriptor;

    descriptor.serial      = get_serial(dev); 
    descriptor.product_id  = desc.idProduct;   
    descriptor.vendor_id   = desc.idVendor;
    descriptor.device_addr = libusb_get_device_address(dev);

    return descriptor;
}


std::string libusb_control_impl::get_serial(libusb_device *dev)
{
    unsigned char buff[32];

    libusb_device_descriptor desc;
    if (libusb_get_device_descriptor(dev, &desc) < 0)
        return "";

    if (desc.iSerialNumber == 0)
        return "";

    //open the device because we have to
    libusb_device_handle *dev_handle;
    if (libusb_open(dev, &dev_handle) < 0)
        return "";

    if (libusb_get_string_descriptor_ascii(dev_handle, desc.iSerialNumber,
                                           buff, sizeof(buff)) < 0) {
        return "";
    }

    libusb_close(dev_handle);

    return (char*) buff;
}


bool libusb_control_impl::open_device()
{
    libusb_device **list;
    libusb_device *dev;

    ssize_t cnt = libusb_get_device_list(_ctx, &list);

    if (cnt < 0)
        return cnt;

    ssize_t i = 0;
    for (i = 0; i < cnt; i++) {
        dev = list[i];
        if (compare_device(dev, _descriptor))
            goto found;
    } 
    return false;

found:
    int ret;
    if ((ret = libusb_open(dev, &_dev_handle)) < 0)
        return false;
    else 
        return true;
}


bool libusb_control_impl::compare_device(libusb_device *dev,
                                         uhd::usb_descriptor_t descriptor)
{
    std::string serial         = descriptor.serial;
    boost::uint16_t vendor_id  = descriptor.vendor_id;
    boost::uint16_t product_id = descriptor.product_id;
    boost::uint8_t device_addr = descriptor.device_addr;

    libusb_device_descriptor libusb_desc;
    if (libusb_get_device_descriptor(dev, &libusb_desc) < 0)
        return false;
    if (serial != get_serial(dev))
        return false;
    if (vendor_id != libusb_desc.idVendor)
        return false;
    if (product_id != libusb_desc.idProduct)
        return false; 
    if (device_addr != libusb_get_device_address(dev))
        return false;

    return true;
}


bool libusb_control_impl::open_interface()
{
    if (libusb_claim_interface(_dev_handle, 0) < 0)
        return false;
    else
        return true;
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
usb_control::sptr usb_control::make(uhd::usb_descriptor_t descriptor)
{
    return sptr(new libusb_control_impl(descriptor));
}

uhd::usb_descriptors_t usb_control::get_device_list()
{
    libusb_device **list;
    uhd::usb_descriptors_t descriptors;

    if (libusb_init(NULL) < 0)
        throw std::runtime_error("USB: failed to initialize libusb");

    ssize_t cnt = libusb_get_device_list(NULL, &list);

    if (cnt < 0)
        throw std::runtime_error("USB: failed to get device list");

    ssize_t i = 0;
    for (i = 0; i < cnt; i++) {
        libusb_device *dev = list[i];
        descriptors.push_back(libusb_control_impl::create_descriptor(dev));
    } 

    libusb_free_device_list(list, 0);
    libusb_exit(NULL);
    return descriptors;
}



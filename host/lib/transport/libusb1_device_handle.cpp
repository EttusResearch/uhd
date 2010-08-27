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
#include <uhd/utils/assert.hpp>

using namespace uhd::transport;

const int libusb_debug_level = 3;

class libusb1_device_handle_impl : public usb_device_handle {
public:
    libusb1_device_handle_impl(std::string serial,
                               boost::uint32_t product_id,
                               boost::uint32_t vendor_id,
                               boost::uint32_t device_addr)
      : _serial(serial), _product_id(product_id), 
        _vendor_id(vendor_id), _device_addr(device_addr)
    {
        /* NOP */
    }

    ~libusb1_device_handle_impl()
    {
        /* NOP */
    }

    std::string get_serial() const
    {
        return _serial;
    }

    boost::uint16_t get_vendor_id() const
    {
        return _vendor_id;
    }


    boost::uint16_t get_product_id() const
    {
        return _product_id;
    }

    boost::uint16_t get_device_addr() const
    {
        return _device_addr;
    }

private:
    std::string     _serial;
    boost::uint32_t _product_id;
    boost::uint32_t _vendor_id;
    boost::uint32_t _device_addr;
};


usb_device_handle::sptr make_usb_device_handle(libusb_device *dev)
{
    libusb_device_descriptor desc;

    if (libusb_get_device_descriptor(dev, &desc) < 0) {
        UHD_ASSERT_THROW("USB: failed to get device descriptor");
    }

    std::string     serial      = libusb::get_serial(dev);
    boost::uint32_t product_id  = desc.idProduct;
    boost::uint32_t vendor_id   = desc.idVendor;
    boost::uint32_t device_addr = libusb_get_device_address(dev);

    return usb_device_handle::sptr(new libusb1_device_handle_impl(
        serial,
        product_id,
        vendor_id,
        device_addr));
}

bool check_fsf_device(libusb_device *dev)
{
    libusb_device_descriptor desc;

    if (libusb_get_device_descriptor(dev, &desc) < 0) {
        UHD_ASSERT_THROW("USB: failed to get device descriptor");
    }

    return desc.idVendor == 0xfffe;
}

std::vector<usb_device_handle::sptr> usb_device_handle::get_device_list()
{
    libusb_context *ctx = NULL;
    libusb_device **list;
    std::vector<usb_device_handle::sptr> device_list;

    libusb::init(&ctx, libusb_debug_level);

    ssize_t cnt = libusb_get_device_list(ctx, &list);

    if (cnt < 0)
        throw std::runtime_error("USB: enumeration failed");

    ssize_t i = 0;
    for (i = 0; i < cnt; i++) {
        libusb_device *dev = list[i];
        if (check_fsf_device(dev)) 
            device_list.push_back(make_usb_device_handle(dev));
    }

    libusb_free_device_list(list, 0);
    libusb_exit(ctx);
    return device_list; 
}

//
// Copyright 2010-2011 Ettus Research LLC
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

#include <uhd/transport/usb_device_handle.hpp>
#include <uhd/transport/usb_control.hpp>
#include <uhd/transport/usb_zero_copy.hpp>
#include <uhd/exception.hpp>

using namespace uhd;
using namespace uhd::transport;

usb_control::~usb_control(void){
    /* NOP */
}

std::vector<usb_device_handle::sptr> usb_device_handle::get_device_list(boost::uint16_t, boost::uint16_t){
    return std::vector<usb_device_handle::sptr>(); //empty list
}

usb_control::sptr usb_control::make(usb_device_handle::sptr, const size_t){
    throw uhd::not_implemented_error("no usb support -> usb_control::make not implemented");
}

usb_zero_copy::sptr usb_zero_copy::make(
    usb_device_handle::sptr, const size_t, const size_t, const size_t, const size_t, const device_addr_t &
){
    throw uhd::not_implemented_error("no usb support -> usb_zero_copy::make not implemented");
}

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

#ifndef INCLUDED_UHD_TYPES_USB_DESCRIPTOR_HPP
#define INCLUDED_UHD_TYPES_USB_DESCRIPTOR_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>
#include <vector>
#include <string>

namespace uhd{

    /*!
     * The USB descriptor struct holds identity information for a USB device
     */
    struct UHD_API usb_descriptor_t{
        std::string serial;
        boost::uint16_t vendor_id;
        boost::uint16_t product_id;
        boost::uint16_t device_addr;

        /*!
         * Create a pretty print string for this USB descriptor struct.
         * \return the printable string
         */
        std::string to_pp_string(void) const;
    };

    //handy typde for a vector of usb descriptors
    typedef std::vector<usb_descriptor_t> usb_descriptors_t;

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_USB_DESCRIPTOR_HPP */

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

#ifndef INCLUDED_UHD_TYPES_DEVICE_ADDR_HPP
#define INCLUDED_UHD_TYPES_DEVICE_ADDR_HPP

#include <uhd/config.hpp>
#include <uhd/types/dict.hpp>
#include <vector>
#include <string>

namespace uhd{

    /*!
     * Mapping of key/value pairs for locating devices on the system.
     * When left empty, the device discovery routines will search
     * all available transports on the system (ethernet, usb...).
     *
     * To narrow down the discovery process to a particular device,
     * specify a transport key/value pair specific to your device.
     * - Ex, to find a usrp2: my_dev_addr["addr"] = [resolvable_hostname_or_ip]
     *
     * The device address can also be used to pass arguments into
     * the transport layer control to set (for example) buffer sizes.
     *
     * An arguments string, is a way to represent a device address
     * using a single string with delimiter characters.
     * - Ex: addr=192.168.10.2
     * - Ex: addr=192.168.10.2, rx_buff_size=1e6
     */
    class UHD_API device_addr_t : public dict<std::string, std::string>{
    public:
        /*!
         * Create a device address from an args string.
         * \param args the arguments string
         */
        device_addr_t(const std::string &args = "");

        /*!
         * Convert a device address into a printable string.
         * \return string good for use with std::cout <<
         */
        std::string to_string(void) const;

        /*!
         * Convert the device address into an args string.
         * The args string contains delimiter symbols.
         * \return a string with delimiter markup
         */
        std::string to_args_str(void) const;
    };

    //handy typedef for a vector of device addresses
    typedef std::vector<device_addr_t> device_addrs_t;

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_DEVICE_ADDR_HPP */

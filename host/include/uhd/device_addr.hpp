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

#ifndef INCLUDED_UHD_DEVICE_ADDR_HPP
#define INCLUDED_UHD_DEVICE_ADDR_HPP

#include <uhd/config.hpp>
#include <uhd/dict.hpp>
#include <boost/cstdint.hpp>
#include <string>
#include <iostream>
#include <vector>

namespace uhd{

    /*!
    * Wrapper for an ethernet mac address.
    * Provides conversion between string and binary formats.
    */
    struct UHD_API mac_addr_t{
        boost::uint8_t mac_addr[6];
        mac_addr_t(const std::string &mac_addr_str = "00:00:00:00:00:00");
        std::string to_string(void) const;
    };

    /*!
     * The device address args are just a mapping of key/value string pairs.
     * When left empty, the discovery routine will try to find all usrps.
     * The discovery can be narrowed down by specifying the transport type arguments.
     *
     * For example, to access a specific usrp2 one would specify the transport type
     * ("type", "udp") and the transport args ("addr", "<resolvable_hostname_or_addr>").
     */
    typedef dict<std::string, std::string> device_addr_t;
    typedef std::vector<device_addr_t> device_addrs_t;

    /*!
     * Function to turn a device address into a string.
     * Just having the operator<< below should be sufficient.
     * However, boost format seems to complain with the %
     * and this is just easier because it works.
     * \param device_addr a device address instance
     * \return the string representation
     */
    namespace device_addr{
        UHD_API std::string to_string(const device_addr_t &device_addr);
    }

} //namespace uhd

//ability to use types with stream operators
UHD_API std::ostream& operator<<(std::ostream &, const uhd::device_addr_t &);
UHD_API std::ostream& operator<<(std::ostream &, const uhd::mac_addr_t &);

#endif /* INCLUDED_UHD_DEVICE_ADDR_HPP */

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
     * The device address args are just a mapping of key/value string pairs.
     * When left empty, the discovery routine will try to find all usrps.
     * The discovery can be narrowed down by specifying the transport type arguments.
     *
     * For example, to access a specific usrp2 one would specify the transport type
     * ("type", "udp") and the transport args ("addr", "<resolvable_hostname_or_addr>").
     */
    class UHD_API device_addr_t : public dict<std::string, std::string>{
        public: std::string to_string(void) const;
    };

    //handy typedef for a vector of device addresses
    typedef std::vector<device_addr_t> device_addrs_t;

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_DEVICE_ADDR_HPP */

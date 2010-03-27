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

#ifndef INCLUDED_UHD_TYPES_MAC_ADDR_HPP
#define INCLUDED_UHD_TYPES_MAC_ADDR_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>
#include <string>

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

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_MAC_ADDR_HPP */

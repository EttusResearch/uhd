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

#ifndef INCLUDED_UHD_TRANSPORT_IF_ADDRS_HPP
#define INCLUDED_UHD_TRANSPORT_IF_ADDRS_HPP

#include <uhd/config.hpp>
#include <string>
#include <vector>

namespace uhd{ namespace transport{

    /*!
     * The address for a network interface.
     */
    struct UHD_API if_addrs_t{
        std::string inet;
        std::string mask;
        std::string bcast;
    };

    /*!
     * Get a list of network interface addresses.
     * The internal implementation is system-dependent.
     * \return a vector of if addrs
     */
    UHD_API std::vector<if_addrs_t> get_if_addrs(void);

}} //namespace


#endif /* INCLUDED_UHD_TRANSPORT_IF_ADDRS_HPP */

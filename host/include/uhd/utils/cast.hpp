//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_UHD_UTILS_CAST_HPP
#define INCLUDED_UHD_UTILS_CAST_HPP

#include <uhd/config.hpp>
#include <string>
#include <sstream>

namespace uhd{ namespace cast{
    //! Convert a hexadecimal string into a value.
    //
    // Example:
    //     boost::uint16_t x = hexstr_cast<boost::uint16_t>("0xDEADBEEF");
    // Uses stringstream.
    template<typename T> inline T hexstr_cast(const std::string &in)
    {
        T x;
        std::stringstream ss;
        ss << std::hex << in;
        ss >> x;
        return x;
    }

}} //namespace uhd::cast

#endif /* INCLUDED_UHD_UTILS_CAST_HPP */


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

#ifndef INCLUDED_UHD_TYPES_ENDIANNESS_HPP
#define INCLUDED_UHD_TYPES_ENDIANNESS_HPP

#include <uhd/config.hpp>

/******************************************************************************
 * Detect host endianness
 *****************************************************************************/
#include <boost/predef/other/endian.h>

// In Boost 1.55, the meaning of the macros changed. They are now always
// defined, but don't always have the same value.
#if BOOST_ENDIAN_BIG_BYTE
#    define UHD_BIG_ENDIAN
#elif BOOST_ENDIAN_LITTLE_BYTE
#    define UHD_LITTLE_ENDIAN
#else
#    error "Unsupported endianness!"
#endif

namespace uhd{

    enum endianness_t {
        ENDIANNESS_BIG,
        ENDIANNESS_LITTLE
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_ENDIANNESS_HPP */

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

#ifndef INCLUDED_UHD_UTILS_BYTESWAP_HPP
#define INCLUDED_UHD_UTILS_BYTESWAP_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>

/*! \file byteswap.hpp
 * Provide fast byteswaping routines for 16, 32, and 64 bit integers,
 * by using the system's native routines/intrinsics when available.
 */

namespace uhd{

    //! perform a byteswap on a 16 bit integer
    boost::uint16_t byteswap(boost::uint16_t);

    //! perform a byteswap on a 32 bit integer
    boost::uint32_t byteswap(boost::uint32_t);

    //! perform a byteswap on a 64 bit integer
    boost::uint64_t byteswap(boost::uint64_t);

} //namespace uhd

/***********************************************************************
 * Platform-specific implementation details for byteswap below:
 **********************************************************************/
#ifdef BOOST_MSVC //http://msdn.microsoft.com/en-us/library/a3140177%28VS.80%29.aspx
    #include <stdlib.h>

    UHD_INLINE boost::uint16_t uhd::byteswap(boost::uint16_t x){
        return _byteswap_ushort(x);
    }

    UHD_INLINE boost::uint32_t uhd::byteswap(boost::uint32_t x){
        return _byteswap_ulong(x);
    }

    UHD_INLINE boost::uint64_t uhd::byteswap(boost::uint64_t x){
        return _byteswap_uint64(x);
    }

#elif defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 3
    #include <byteswap.h>

    UHD_INLINE boost::uint16_t uhd::byteswap(boost::uint16_t x){
        return bswap_16(x); //no __builtin_bswap16
    }

    UHD_INLINE boost::uint32_t uhd::byteswap(boost::uint32_t x){
        return __builtin_bswap32(x);
    }

    UHD_INLINE boost::uint64_t uhd::byteswap(boost::uint64_t x){
        return __builtin_bswap64(x);
    }

#else
    #include <byteswap.h>

    UHD_INLINE boost::uint16_t uhd::byteswap(boost::uint16_t x){
        return bswap_16(x);
    }

    UHD_INLINE boost::uint32_t uhd::byteswap(boost::uint32_t x){
        return bswap_32(x);
    }

    UHD_INLINE boost::uint64_t uhd::byteswap(boost::uint64_t x){
        return bswap_64(x);
    }

#endif

#endif /* INCLUDED_UHD_UTILS_BYTESWAP_HPP */

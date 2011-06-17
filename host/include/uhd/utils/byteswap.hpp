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

    //! network to host: short, long, or long-long
    template<typename T> T ntohx(T);

    //! host to network: short, long, or long-long
    template<typename T> T htonx(T);

    //! worknet to host: short, long, or long-long
    template<typename T> T wtohx(T);

    //! host to worknet: short, long, or long-long
    template<typename T> T htowx(T);

} //namespace uhd

#include <uhd/utils/byteswap.ipp>

#endif /* INCLUDED_UHD_UTILS_BYTESWAP_HPP */

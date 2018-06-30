//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_BYTESWAP_HPP
#define INCLUDED_UHD_UTILS_BYTESWAP_HPP

#include <uhd/config.hpp>
#include <stdint.h>

/*! \file byteswap.hpp
 *
 * Provide fast byteswaping routines for 16, 32, and 64 bit integers,
 * by using the system's native routines/intrinsics when available.
 */
namespace uhd{

    //! perform a byteswap on a 16 bit integer
    uint16_t byteswap(uint16_t);

    //! perform a byteswap on a 32 bit integer
    uint32_t byteswap(uint32_t);

    //! perform a byteswap on a 64 bit integer
    uint64_t byteswap(uint64_t);

    //! network to host: short, long, or long-long
    template<typename T> T ntohx(T);

    //! host to network: short, long, or long-long
    template<typename T> T htonx(T);

    //! worknet to host: short, long, or long-long
    //
    // The argument is assumed to be little-endian (i.e, the inverse
    // of typical network endianness).
    template<typename T> T wtohx(T);

    //! host to worknet: short, long, or long-long
    //
    // The return value is little-endian (i.e, the inverse
    // of typical network endianness).
    template<typename T> T htowx(T);

} //namespace uhd

#include <uhd/utils/byteswap.ipp>

#endif /* INCLUDED_UHD_UTILS_BYTESWAP_HPP */

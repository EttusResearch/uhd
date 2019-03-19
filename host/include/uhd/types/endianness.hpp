//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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


namespace uhd {

enum endianness_t { ENDIANNESS_BIG, ENDIANNESS_LITTLE };

} // namespace uhd

#endif /* INCLUDED_UHD_TYPES_ENDIANNESS_HPP */

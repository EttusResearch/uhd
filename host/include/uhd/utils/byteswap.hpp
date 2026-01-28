//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/types/endianness.hpp>
#include <stdint.h>

/*! \file byteswap.hpp
 *
 * Provide fast byteswapping routines for 16, 32, and 64 bit integers,
 * by using the system's native routines/intrinsics when available.
 */
namespace uhd {

//! perform a byteswap on a 16 bit integer
uint16_t byteswap(uint16_t);

//! perform a byteswap on a 32 bit integer
uint32_t byteswap(uint32_t);

//! perform a byteswap on a 64 bit integer
uint64_t byteswap(uint64_t);

//! network to host: short, long, or long-long
template <typename T>
T ntohx(T);

//! host to network: short, long, or long-long
template <typename T>
T htonx(T);

//! worknet to host: short, long, or long-long
//
// The argument is assumed to be little-endian (i.e, the inverse
// of typical network endianness).
template <typename T>
T wtohx(T);

//! host to worknet: short, long, or long-long
//
// The return value is little-endian (i.e, the inverse
// of typical network endianness).
template <typename T>
T htowx(T);

//! Host to link with configurable endianness (16-bit, 32-bit, or 64-bit).
//
// Use this if the link endianness ("network or worknet") is configurable as
// template parameter. Note the host endianness is auto-detected at compile
// time.
//
// \tparam endianness The link endianness.
// \param num The value in host endianness, to be swapped.
// \return The value byte-swapped to the link endianness.
template <endianness_t endianness, typename T>
T htolx(T num);

//! Link with configurable endianness to host (16-bit, 32-bit, or 64-bit).
//
// Use this if the link endianness ("network or worknet") is configurable as
// template parameter. Note the host endianness is auto-detected at compile
// time.
//
// \tparam endianness The link endianness.
// \param num The value in link endianness, to be swapped.
// \return The value byte-swapped to the host endianness.
template <endianness_t endianness, typename T>
T ltohx(T num);

} // namespace uhd

#include <uhd/utils/byteswap.ipp>

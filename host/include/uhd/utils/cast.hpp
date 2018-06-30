//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
    //     uint16_t x = hexstr_cast<uint16_t>("0xDEADBEEF");
    // Uses stringstream.
    template<typename T> UHD_INLINE T hexstr_cast(const std::string &in)
    {
        T x;
        std::stringstream ss;
        ss << std::hex << in;
        ss >> x;
        return x;
    }

}} //namespace uhd::cast

#endif /* INCLUDED_UHD_UTILS_CAST_HPP */


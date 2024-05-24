//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <iomanip>
#include <sstream>
#include <string>

namespace uhd { namespace cast {
//! Convert a hexadecimal string into a value.
//
// Example:
//     uint16_t x = hexstr_cast<uint16_t>("0xDEADBEEF");
// Uses stringstream.
template <typename T>
inline T hexstr_cast(const std::string& in)
{
    T x;
    std::stringstream ss;
    ss << std::hex << in;
    ss >> x;
    return x;
}

//! Convert hexadecimal, decimal, octal or other strings that support the >>
//! operator into a value depending on the prefix.
//
// Example:
//     10, 0x10, 010 get parsed to decimal 10, 16, 8.
//     uint32_t x = fromstr_cast<uint32_t>("0xaffe");
// Uses istringstream.
template <typename T>
inline T fromstr_cast(const std::string& in)
{
    T x;
    std::istringstream is(in);
    is >> std::setbase(0) >> x;
    return x;
}

//! Generic cast-from-string function
template <typename data_t>
data_t from_str(const std::string&)
{
    throw uhd::runtime_error("Cannot convert from string!");
}

// Specializations of `uhd::cast::from_str()` for supported data types

//! Specialization of `uhd::cast::from_str()` for Boolean values
//
//   Examples evaluating to `true`: 'True', 'Yes', 'y', '1', empty string
//   Examples evaluating to `false`: 'false', 'NO', 'n', '0'
//   Throws `uhd::runtime_error` if the string can't be converted to `bool`
template <>
UHD_API bool from_str(const std::string& val);

//! Specialization of `uhd::cast::from_str()` for double-precision values
template <>
UHD_API double from_str(const std::string& val);

//! Specialization of `uhd::cast::from_str()` for integer values
template <>
UHD_API int from_str(const std::string& val);

//! Specialization of `uhd::cast::from_str()` for strings
//
//   This function simply returns the incoming string
template <>
UHD_API std::string from_str(const std::string& val);

//! Create an ordinal string from a number.
UHD_API std::string to_ordinal_string(int val);

}} // namespace uhd::cast

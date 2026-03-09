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
#include <type_traits>
#include <cstdint>
#include <iomanip>
#include <ios>
#include <limits>
#include <locale>
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

//! Specialization of `uhd::cast::from_str()` for size_t values
template <>
UHD_API size_t from_str(const std::string& val);

//! Specialization of `uhd::cast::from_str()` for strings
//
//   This function simply returns the incoming string
template <>
UHD_API std::string from_str(const std::string& val);

//! Create an ordinal string from a number.
UHD_API std::string to_ordinal_string(int val);

//! Template implementations for `uhd::cast::to_str()` for common types
//
// These template implementations provide consistent string conversion
// for integer and floating-point types commonly used in UHD, particularly
// for device_addr_t parameter handling.

// Type trait to detect if a type has a to_string() method
namespace detail {
template <typename T>
auto has_to_string_method_impl(int)
    -> decltype(std::declval<T>().to_string(), std::true_type{});
template <typename T>
auto has_to_string_method_impl(...) -> std::false_type;
template <typename T>
using has_to_string_method = decltype(has_to_string_method_impl<T>(0));
} // namespace detail

//! SFINAE-based template for integer types that std::to_string can handle
//
// This template will automatically work for integer types that std::to_string supports
// (int, unsigned int, long, unsigned long, long long, unsigned long long, size_t, etc.),
// excluding int8_t and uint8_t which need special handling, and floating-point types
// which need precision handling, and excluding types that have their own to_string()
// method.
template <typename T>
auto to_str(const T& val) -> std::enable_if_t<
    std::is_arithmetic_v<
        T> && !std::is_floating_point_v<T> && !std::is_same_v<T, int8_t> && !std::is_same_v<T, uint8_t> && !detail::has_to_string_method<T>::value,
    decltype(std::to_string(val))>
{
    return std::to_string(val);
}

//! Template for floating-point types with round-trip safe precision
//
// Uses ostringstream with max_digits10 precision and classic locale to ensure
// round-trip safety for floating-point values.
template <typename T>
auto to_str(const T& val) -> std::enable_if_t<
    std::is_floating_point_v<T> && !detail::has_to_string_method<T>::value,
    std::string>
{
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss.precision(std::numeric_limits<T>::max_digits10);
    oss << val;
    return oss.str();
}

// Special handling for int8_t and uint8_t since std::to_string treats them as chars

//! Overload of `uhd::cast::to_str()` for int8_t
inline std::string to_str(const int8_t& val)
{
    return std::to_string(static_cast<int>(val));
}

//! Overload of `uhd::cast::to_str()` for uint8_t
inline std::string to_str(const uint8_t& val)
{
    return std::to_string(static_cast<unsigned int>(val));
}

//! SFINAE-based template for any type that has a to_string() method
//
// This template will automatically work for any type T that has a to_string()
// method that returns a type convertible to std::string.
template <typename T>
auto to_str(const T& val)
    -> std::enable_if_t<std::is_convertible_v<decltype(val.to_string()), std::string>,
        std::string>
{
    return val.to_string();
}

//! Overload for std::string and C-style strings
inline std::string to_str(const std::string& val)
{
    return val;
}

inline std::string to_str(const char* val)
{
    return std::string(val);
}

}} // namespace uhd::cast

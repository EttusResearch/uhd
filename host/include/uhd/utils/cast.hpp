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
#include <complex>
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
//
// Handles size_t aliases (unsigned long, unsigned long long) via if constexpr
// when they are not the same type as uint32_t or uint64_t.
// Explicit specializations below override this for bool, double, int,
// uint32_t, uint64_t, uint16_t, uint8_t, and std::string.
//
// Note that we need this generic template to handle arbitrary types at runtime.
// However, this means we can't use SFINAE to create template specializations,
// which means any type that requires template logic needs to go into here and
// must be explicitly handled with if constexpr.
template <typename data_t>
data_t from_str(const std::string& val)
{
    if constexpr (
        (std::is_same_v<data_t,
             size_t> || std::is_same_v<data_t, unsigned long> || std::is_same_v<data_t, unsigned long long>)&&!std::
            is_same_v<data_t, uint32_t> && !std::is_same_v<data_t, uint64_t>) {
        try {
            if (!val.empty() && val[0] == '-') {
                throw std::out_of_range("negative value for unsigned type");
            }
            size_t pos;
            unsigned long long tmp = std::stoull(val, &pos);
            if (pos != val.length()) {
                throw std::invalid_argument("trailing characters");
            }
            if (tmp > std::numeric_limits<data_t>::max()) {
                throw std::out_of_range("value out of range");
            }
            return static_cast<data_t>(tmp);
        } catch (std::invalid_argument&) {
            throw uhd::runtime_error(
                std::string("Cannot convert `") + val + "' to numeric type!");
        } catch (std::out_of_range&) {
            throw uhd::runtime_error(
                std::string("Cannot convert `") + val + "' to numeric type!");
        }
    } else {
        throw uhd::runtime_error("Cannot convert from string!");
    }
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

//! Specialization of `uhd::cast::from_str()` for uint8_t values
template <>
UHD_API uint8_t from_str(const std::string& val);

//! Specialization of `uhd::cast::from_str()` for uint16_t values
template <>
UHD_API uint16_t from_str(const std::string& val);

//! Specialization of `uhd::cast::from_str()` for uint32_t values
template <>
UHD_API uint32_t from_str(const std::string& val);

//! Specialization of `uhd::cast::from_str()` for uint64_t values
template <>
UHD_API uint64_t from_str(const std::string& val);

//! Specialization of `uhd::cast::from_str()` for strings
template <>
inline UHD_API std::string from_str(const std::string& val)
{
    return val;
}

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
        T> && !std::is_floating_point_v<T> && !std::is_same_v<T, int8_t> && !std::is_same_v<T, uint8_t> && !std::is_enum_v<T> && !detail::has_to_string_method<T>::value,
    decltype(std::to_string(val))>
{
    return std::to_string(val);
}

//! SFINAE-based template for enum types
//
// Convert enum types to their underlying integral type, then to string
template <typename T>
auto to_str(const T& val) -> std::enable_if_t<std::is_enum_v<T>, std::string>
{
    return std::to_string(static_cast<std::underlying_type_t<T>>(val));
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

//! Template specialization for int8_t
template <typename T>
auto to_str(const T& val)
    -> std::enable_if_t<std::is_same_v<T, int8_t> || std::is_same_v<T, signed char>,
        std::string>
{
    return std::to_string(static_cast<int>(val));
}

//! Template specialization for uint8_t
template <typename T>
auto to_str(const T& val)
    -> std::enable_if_t<std::is_same_v<T, uint8_t> || std::is_same_v<T, unsigned char>,
        std::string>
{
    return std::to_string(static_cast<unsigned int>(val));
}

//! SFINAE-based template for std::complex<T> types
//
// This template will work for any std::complex<T> where T has a to_str() function
// Creates a string of the form "a+jb" or "a-jb"
template <typename T>
std::string to_str(const std::complex<T>& val)
{
    std::string real_str = to_str(val.real());
    T imag_val           = val.imag();

    if (imag_val >= 0) {
        return real_str + "+j" + to_str(imag_val);
    } else {
        return real_str + "-j" + to_str(-imag_val);
    }
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

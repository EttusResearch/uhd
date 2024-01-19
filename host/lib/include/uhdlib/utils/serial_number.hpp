//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHDLIB_UTILS_SERIAL_NUMBER_HPP
#define INCLUDED_UHDLIB_UTILS_SERIAL_NUMBER_HPP

#include <string>

namespace uhd { namespace utils {

/*! Convenience function to determine whether two serial numbers refer to the
 * same device. Serial numbers are case-insensitive and ignore leading zeros,
 * so e.g. the strings "0123abcd" and "123ABCD" are considered the same serial
 * number.
 *
 * Serial numbers cannot be longer than 8 characters or have characters outside
 * the range 0-9a-fA-F.
 *
 * \param serial_a The first serial number to compare
 * \param serial_b The second serial number to compare
 */
bool serial_numbers_match(const std::string& serial_a, const std::string& serial_b);

}} // namespace uhd::utils

#endif /* INCLUDED_UHDLIB_UTILS_SERIAL_NUMBER_HPP */

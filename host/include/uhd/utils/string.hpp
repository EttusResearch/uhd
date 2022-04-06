//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/exception.hpp>

namespace uhd { namespace string {

/*!
 * Split a string using a delimiter. The delimiter must be a substring of the
 * original string.
 *
 * \param str the string to be split
 * \param delim the delimiter in str used to determine the position of the split
 * \return a pair of substrings containing the characters before the first
 *         instance of the delimiter and after the first instance of the delimiter
 * \throws uhd::runtime_error if the delimiter is not found in the original string
 */
UHD_API std::pair<std::string, std::string> split(
    const std::string& str, const std::string& delim)
{
    auto delim_pos = str.find(delim);
    if (delim_pos == std::string::npos) {
        throw uhd::runtime_error(
            "Delimiter \"" + delim + "\" not found in string \"" + str + "\"");
    }
    return std::make_pair(str.substr(0, delim_pos), str.substr(delim_pos + 1));
}

}} // namespace uhd::string

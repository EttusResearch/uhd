//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/cast.hpp>
#include <boost/format.hpp>

namespace uhd {

template <typename T, typename Range>
UHD_INLINE void assert_has(const Range& range, const T& value, const std::string& what)
{
    if (uhd::has(range, value))
        return;
    std::string possible_values = "";
    size_t i                    = 0;
    for (const T& v : range) {
        if (i++ > 0)
            possible_values += ", ";
        possible_values += uhd::cast::to_str(v);
    }
    throw uhd::assertion_error(
        boost::str(boost::format("assertion failed:\n"
                                 "  %s is not a valid %s.\n"
                                 "  possible values are: [%s].\n")
                   % uhd::cast::to_str(value) % what % possible_values));
}

} // namespace uhd

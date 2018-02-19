//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_ASSERT_HAS_HPP
#define INCLUDED_UHD_UTILS_ASSERT_HAS_HPP

#include <uhd/config.hpp>
#include <string>

namespace uhd{

    /*!
     * Check that an element is found in a container.
     * If not, throw a meaningful assertion error.
     * The "what" in the error will show what is
     * being set and a list of known good values.
     *
     * \param range a list of possible settings
     * \param value an element that may be in the list
     * \param what a description of what the value is
     * \throw assertion_error when elem not in list
     */
    template<typename T, typename Range> void assert_has(
        const Range &range,
        const T &value,
        const std::string &what = "unknown"
    );

}//namespace uhd

#include <uhd/utils/assert_has.ipp>

#endif /* INCLUDED_UHD_UTILS_ASSERT_HAS_HPP */

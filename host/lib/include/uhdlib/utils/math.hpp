//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// More math, but not meant for public API

#ifndef INCLUDED_UHDLIB_UTILS_MATH_HPP
#define INCLUDED_UHDLIB_UTILS_MATH_HPP

#include <cmath>

namespace uhd { namespace math {

/*! log2(num), rounded up to the nearest integer.
 */
template <class T>
T ceil_log2(T num){
    return std::ceil(std::log(num)/std::log(T(2)));
}

}} /* namespace uhd::math */

#endif /* INCLUDED_UHDLIB_UTILS_MATH_HPP */

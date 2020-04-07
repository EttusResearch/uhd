//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// More math, but not meant for public API

#pragma once

#include <uhdlib/utils/narrow.hpp>
#include <cmath>
#include <vector>

namespace uhd { namespace math {

/*! log2(num), rounded up to the nearest integer.
 */
template <class T>
T ceil_log2(T num)
{
    return std::ceil(std::log(num) / std::log(T(2)));
}

/**
 * Function which attempts to find integer values a and b such that
 * a / b approximates the decimal represented by f within max_error and
 * b is not greater than maximum_denominator.
 *
 * If the approximation cannot achieve the desired error without exceeding
 * the maximum denominator, b is set to the maximum value and a is set to
 * the closest value.
 *
 * @param f is a positive decimal to be converted, must be between 0 and 1
 * @param maximum_denominator maximum value allowed for b
 * @param max_error how close to f the expression a / b should be
 */
template <typename IntegerType>
std::pair<IntegerType, IntegerType> rational_approximation(
    const double f, const IntegerType maximum_denominator, const double max_error)
{
    static constexpr IntegerType MIN_DENOM     = 1;
    static constexpr size_t MAX_APPROXIMATIONS = 64;

    UHD_ASSERT_THROW(maximum_denominator <= std::numeric_limits<IntegerType>::max());
    UHD_ASSERT_THROW(f < 1 and f >= 0);

    // This function uses a successive approximations formula to attempt to
    // find a "best" rational to use to represent a decimal. This algorithm
    // finds a continued fraction of up to 64 terms, or such that the last
    // term is less than max_error

    if (f < max_error) {
        return {0, MIN_DENOM};
    }

    double c                         = f;
    std::vector<double> saved_denoms = {c};

    // Create the continued fraction by taking the reciprocal of the
    // fractional part, expressing the denominator as a mixed number,
    // then repeating the algorithm on the fractional part of that mixed
    // number until a maximum number of terms or the fractional part is
    // nearly zero.
    for (size_t i = 0; i < MAX_APPROXIMATIONS; ++i) {
        double x = std::floor(1.0 / c);
        c        = (1.0 / c) - x;

        saved_denoms.push_back(x);

        if (std::abs(c) < max_error)
            break;
    }

    double num   = 1.0;
    double denom = saved_denoms.back();

    // Calculate a single rational which will be equivalent to the
    // continued fraction created earlier.  Because the continued fraction
    // is composed of only integers, the final rational will be as well.
    for (auto it = saved_denoms.rbegin() + 1; it != saved_denoms.rend() - 1; ++it) {
        double new_denom = denom * (*it) + num;
        if (new_denom > maximum_denominator) {
            // We can't do any better than using the maximum denominator
            num   = std::round(f * maximum_denominator);
            denom = maximum_denominator;
            break;
        }

        num   = denom;
        denom = new_denom;
    }

    return {uhd::narrow<IntegerType>(num), uhd::narrow<IntegerType>(denom)};
}


}} /* namespace uhd::math */

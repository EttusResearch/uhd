//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Various interpolation functions used within UHD

#ifndef INCLUDED_UHD_UTILS_INTERP_HPP
#define INCLUDED_UHD_UTILS_INTERP_HPP

#include <uhd/utils/math.hpp>
#include <uhd/utils/interpolation.hpp>
#include <map>

namespace uhd { namespace math {

//! Linearly interpolate f(x) given f(x0) = y0 and f(x1) = y1
//
// This draws a line through the coordinates x0/y0 and x1/y1, and then returns
// the y-value for the given x-value on said line.
//
// \throws uhd::runtime_error if x0 == x1, since that doesn't allow us to
//         interpolate.
template <typename InterpType>
inline InterpType linear_interp(
    InterpType x, InterpType x0, InterpType y0, InterpType x1, InterpType y1)
{
    if (x0 == x1) {
        throw uhd::runtime_error("linear_interp(): x0 and x1 must differ!");
    }
    return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

/*! Like std::at, but will interpolate the data between two points
 *
 * Most often, this is used with a std::map (keys need to be sorted!). Unlike a
 * regular map, this function allows looking up values in the map even if they're
 * not a data point, by interpolating between the two data points that are closest
 * to the given key.
 *
 * Example:
 * ~~~{.cpp}
 * std::map<double, double> data{{1.0, 2.0}, {2.0, 3.0}};
 * std::cout << data[1.5] << std::endl; // This will fail!
 * // This will print something depending on my_interpolation_func:
 * std::cout << at_interpolate_1d(data, 1.5, &my_interpolation_func) << std::endl;
 * ~~~
 *
 * Note: When \p key exceeds the max key of \p data, or is lower than the min key
 * of \p data, it will return the value for the max or min key, respectively.
 *
 * \param data A map that stores x/y values. It must implemented lower_bound().
 * \param key The x-value to look up
 * \param interp_func A function that takes 5 inputs (x, x0, y0, x1, y1) and
 *                    returns f(x) for f(x0) == y0 and f(x1) == y1. The inputs
 *                    x, x0, and x1 must be of the key type of \p data, and the
 *                    return value must be of the mapped type of \p data.
 * \returns An interpolation of the data at key \p key. The return value type
 *          is the mapped type of \p data.
 */
template <typename map_type, typename interp_func_type>
typename map_type::mapped_type at_interpolate_1d(const map_type& data,
    const typename map_type::key_type& key,
    interp_func_type&& interp_func)
{
    // Get an iterator to the next item
    const auto next_it = data.lower_bound(key);
    if (next_it == data.cend()) {
        // This means key is larger than our biggest key in data, and thus we
        // return the value of the largest key.
        return data.crbegin()->second;
    }
    if (next_it == data.cbegin()) {
        // This means freq is smaller than our smallest key, and thus we
        // return the value of the smallest key.
        return data.cbegin()->second;
    }

    // Get an iterator to the previous item
    auto prev_it = next_it;
    prev_it--;
    const auto hi_key   = next_it->first;
    const auto hi_value = next_it->second;
    const auto lo_key   = prev_it->first;
    const auto lo_value = prev_it->second;

    return interp_func(key, lo_key, lo_value, hi_key, hi_value);
}

//! Like std::map::at, except with an approximate index
//
// Example:
// ~~~{.cpp}
// std::map<double, double> data{{1.0, 2.0}, {2.0, 3.0}};
// std::cout << at_nearest(data, 1.72) << std::endl; // prints 3.0
// ~~~
//
// This is in fact a shorthand for at_interpolate_1d(). It will look up the
// value in \p data with the key that most closely matches \p key, i.e.,
// at_nearest(data, key) == data[key'] if key' == argmin abs(key' - key).
template <typename map_type>
typename map_type::mapped_type at_nearest(
    const map_type& data, const typename map_type::key_type& key)
{
    return at_interpolate_1d(
        data,
        key,
        [&](const typename map_type::key_type x,
            const typename map_type::key_type x0,
            const typename map_type::mapped_type y0,
            const typename map_type::key_type x1,
            const typename map_type::mapped_type y1) ->
        typename map_type::mapped_type { return (x1 - x < x - x0) ? y1 : y0; });
}

//! Like std::map::at, except it will linearly interpolate in one dimension
//
// Example:
// ~~~{.cpp}
// std::map<double, double> data{{1.0, 2.0}, {2.0, 3.0}};
// std::cout << at_lin_interp(data, 1.5) << std::endl; // prints 2.5
// ~~~
//
// This treats the map as a set of x/y coordinates, and returns the value from
// the map that corresponds to a linear interpolation on those coordinates.
//
// For x-values greater than the maximum key, or smaller than the minimum key
// of \p data, we return the value for the closest available key.
template <typename map_type>
typename map_type::mapped_type at_lin_interp(
    const map_type& data, const typename map_type::key_type& key)
{
    return at_interpolate_1d(
        data, key, &uhd::math::linear_interp<typename map_type::mapped_type>);
}

}} // namespace uhd::math

#endif /* INCLUDED_UHD_UTILS_INTERP_HPP */

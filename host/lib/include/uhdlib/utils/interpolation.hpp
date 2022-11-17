//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// Various interpolation functions used within UHD

#pragma once

#include <uhd/utils/interpolation.hpp>
#include <uhd/utils/math.hpp>
#include <map>
#include <utility>

namespace uhd { namespace math {

//! Return a pair of iterators before and after a key within a map.
//
// Complexity: That of std::map::lower_bound (logarithmic).
//
// If \p key is lower or greater than the range of \p data then both the
// returned iterators will point to the first or last item in the map, respectively.
// If the key is found exactly in the map, then the second iterator will point to
// to that key, and the first iterator will point to the previous item (if
// possible).
template <typename map_type>
std::pair<typename map_type::const_iterator, typename map_type::const_iterator>
get_bounding_iterators(const map_type& data, const typename map_type::key_type& key)
{
    // Get an iterator to the next bigger item
    auto next_it = data.lower_bound(key);
    if (next_it == data.end()) {
        // This means key is larger than our biggest key in data's first
        // dimension, and thus we return the value of the largest key.
        next_it--;
        return {next_it, next_it};
    }
    auto prev_it = next_it;
    // If the next if clause is true, then x2 is already the smallest x value,
    // and we keep x1 == x2. Otherwise, we make x1 the next smaller available
    // x value.
    if (prev_it != data.begin()) {
        prev_it--;
    }

    return {prev_it, next_it};
}


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

//! Bi-Linearly interpolate f(x, y) given f(xi, yk) = zik and for i, k == 0, 0
//
// This does one linear interpolation in x-direction and one in y-direction to
// return the z-value for the given x-value and y-value.
//
// \throws uhd::runtime_error if x0 == x1, or y0 == y1 since that doesn't allow
//         us to interpolate.
template <typename InterpType>
inline InterpType bilinear_interp(InterpType x,
    InterpType y,
    InterpType x0,
    InterpType y0,
    InterpType x1,
    InterpType y1,
    InterpType z00,
    InterpType z01,
    InterpType z10,
    InterpType z11)
{
    if (x0 == x1) {
        throw uhd::runtime_error("bilinear_interp(): x0 and x1 must differ!");
    }
    if (y0 == y1) {
        throw uhd::runtime_error("bilinear_interp(): y0 and y1 must differ!");
    }

    return linear_interp(y,
        y0,
        linear_interp(x, x0, z00, x1, z10),
        y1,
        linear_interp(x, x0, z01, x1, z11));
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
    return at_interpolate_1d(data,
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

//! Like std::map::at, except it will do a bilinear interpolation in two dimensions
//
// Example:
// ~~~{.cpp}
// std::map<double, std::map<double, double>> data;
// data[1.0][1.0] = 0.0;
// data[1.0][2.0] = 1.0;
// data[2.0][1.0] = 1.0;
// data[2.0][2.0] = 2.0;
// std::cout << at_bilin_interp(data, 1.5, 1.5) << std::endl; // prints 1.0
// ~~~
//
// This treats the double-map as a set of x/y/z coordinates, and returns the
// value from the map that corresponds to a bilinear interpolation on those
// coordinates.
//
// For x- or y-values greater than the maximum key, or smaller than the minimum
// key of \p data, we return the value for the closest available key.
template <typename doublemap_type>
typename doublemap_type::mapped_type::mapped_type at_bilin_interp(
    const doublemap_type& data,
    const typename doublemap_type::key_type& key_x,
    const typename doublemap_type::mapped_type::key_type& key_y)
{
    // Find x1 and x2 coordinates. They are the x-values closest to key_x.
    const auto x_iters = get_bounding_iterators(data, key_x);
    const auto x1      = x_iters.first->first;
    const auto x2      = x_iters.second->first;
    // x-boundary condition
    if (x1 == x2) {
        return at_lin_interp(x_iters.first->second, key_y);
    }
    // Find y1 and y2 coordinates. They are the y-values closest to key_y.
    const auto y_iters = get_bounding_iterators(x_iters.first->second, key_y);
    const auto y1      = y_iters.first->first;
    const auto y2      = y_iters.second->first;
    // y-boundary condition
    if (y1 == y2) {
        return linear_interp(key_x, x1, data.at(x1).at(y1), x2, data.at(x2).at(y1));
    }

    // Find z values
    const auto z11 = data.at(x1).at(y1);
    const auto z12 = data.at(x1).at(y2);
    const auto z21 = data.at(x2).at(y1);
    const auto z22 = data.at(x2).at(y2);

    return bilinear_interp(key_x, key_y, x1, y1, x2, y2, z11, z12, z21, z22);
}

}} // namespace uhd::math

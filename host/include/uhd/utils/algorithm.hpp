//
// Copyright 2010-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_UTILS_ALGORITHM_HPP
#define INCLUDED_UHD_UTILS_ALGORITHM_HPP

#include <algorithm>
#include <stdint.h>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

/*!
 * Useful templated functions, classes, and constants. Some of these overlap
 * with the STL, but these are created with Boost for portability.
 * Many of the range wrapper functions come with versions of boost >= 1.43.
 */
namespace uhd{
    /*!
     * A wrapper around std::sort that takes a range instead of an iterator.
     *
     * The elements are sorted into ascending order using the less-than operator.
     * This wrapper sorts the elements non-destructively into a new range.
     * Based on the builtin python function sorted(...)
     *
     * \param range the range of elements to be sorted
     * \return a new range with the elements sorted
     */
    template<typename Range> UHD_INLINE Range sorted(const Range &range){
        Range r(range); std::sort(boost::begin(r), boost::end(r)); return r;
    }

    /*!
     * A wrapper around std::reverse that takes a range instead of an iterator.
     *
     * The elements are reversed into descending order using the less-than operator.
     * This wrapper reverses the elements non-destructively into a new range.
     * Based on the builtin python function reversed(...)
     *
     * \param range the range of elements to be reversed
     * \return a new range with the elements reversed
     */
    template<typename Range> UHD_INLINE Range reversed(const Range &range){
        Range r(range); std::reverse(boost::begin(r), boost::end(r)); return r;
    }

    /*!
     * Is the value found within the elements in this range?
     *
     * Uses std::find to search the iterable for an element.
     *
     * \param range the elements to search through
     * \param value the match to look for in the range
     * \return true when the value is found in the range
     */
    template<typename Range, typename T> UHD_INLINE
    bool has(const Range &range, const T &value){
        return boost::end(range) != std::find(boost::begin(range), boost::end(range), value);
    }

    /*!
     * A templated clip implementation.
     * \param val the value to clip between an upper and lower limit
     * \param bound1 the upper or lower bound
     * \param bound2 the upper or lower bound
     * \return the value clipped at the bounds
     */
    template<typename T> UHD_INLINE T clip(const T &val, const T &bound1, const T &bound2){
        const T minimum = std::min(bound1, bound2);
        if (val < minimum) return minimum;
        const T maximum = std::max(bound1, bound2);
        if (val > maximum) return maximum;
        return val;
    }

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_ALGORITHM_HPP */

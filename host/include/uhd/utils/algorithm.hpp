//
// Copyright 2010-2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_UHD_UTILS_ALGORITHM_HPP
#define INCLUDED_UHD_UTILS_ALGORITHM_HPP

#include <algorithm>
#include <boost/range/begin.hpp>
#include <boost/range/end.hpp>

/*!
 * Useful templated functions and classes that I like to pretend are part of stl.
 * Many of the range wrapper functions come with recent versions of boost (1.43).
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
    template<typename Range> inline Range sorted(const Range &range){
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
    template<typename Range> inline Range reversed(const Range &range){
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
    template<typename Range, typename T> inline
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
    template<typename T> inline T clip(const T &val, const T &bound1, const T &bound2){
        const T minimum = std::min(bound1, bound2);
        if (val < minimum) return minimum;
        const T maximum = std::max(bound1, bound2);
        if (val > maximum) return maximum;
        return val;
    }

} //namespace uhd

#endif /* INCLUDED_UHD_UTILS_ALGORITHM_HPP */

//
// Copyright 2010 Ettus Research LLC
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
#include <boost/range/functions.hpp>

/*!
 * Useful templated functions and classes that I like to pretend are part of stl
 */
namespace std{

    template<typename Range, typename T> inline
    bool has(const Range &range, const T &value){
        return boost::end(range) != std::find(boost::begin(range), boost::end(range), value);
    }

    template<typename T> inline T signum(T n){
        if (n < 0) return -1;
        if (n > 0) return 1;
        return 0;
    }

    template<typename T> inline T clip(T val, T minVal, T maxVal){
        return std::min(std::max(val, minVal), maxVal);
    }

}//namespace std

#endif /* INCLUDED_UHD_UTILS_ALGORITHM_HPP */

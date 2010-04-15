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

/*!
 * Useful templated functions and classes that I like to pretend are part of stl
 */
namespace std{

    template<class T, class InputIterator, class Function>
    T reduce(InputIterator first, InputIterator last, Function fcn, T init = 0){
        T tmp = init;
        for ( ; first != last; ++first ){
            tmp = fcn(tmp, *first);
        }
        return tmp;
    }

    template<class T, class Iterable, class Function>
    T reduce(Iterable iterable, Function fcn, T init = 0){
        return reduce(iterable.begin(), iterable.end(), fcn, init);
    }

    template<class T, class InputIterator>
    bool has(InputIterator first, InputIterator last, const T &elem){
        return last != std::find(first, last, elem);
    }

    template<class T, class Iterable>
    bool has(const Iterable &iterable, const T &elem){
        return has(iterable.begin(), iterable.end(), elem);
    }

    template<class T> T signum(T n){
        if (n < 0) return -1;
        if (n > 0) return 1;
        return 0;
    }

    template<class T, class T1, class T2> T clip(T val, T1 minVal, T2 maxVal){
        return std::min<T>(std::max<T>(val, minVal), maxVal);
    }

}//namespace std

#endif /* INCLUDED_UHD_UTILS_ALGORITHM_HPP */

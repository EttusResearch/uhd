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

#ifndef INCLUDED_UHD_TYPES_RANGES_IPP
#define INCLUDED_UHD_TYPES_RANGES_IPP

#include <boost/math/special_functions/round.hpp>
#include <algorithm>
#include <stdexcept>
#include <iostream>

namespace uhd{

    /*******************************************************************
     * range_t implementation code
     ******************************************************************/
    template <typename T> struct range_t<T>::impl{
        impl(const T &start, const T &stop, const T &step):
            start(start), stop(stop), step(step)
        {
            /* NOP */
        }
        const T start, stop, step;
    };

    template <typename T> range_t<T>::range_t(const T &value):
        _impl(UHD_PIMPL_MAKE(impl, (value, value, T(0))))
    {
        /* NOP */
    }

    template <typename T> range_t<T>::range_t(
        const T &start, const T &stop, const T &step
    ):
        _impl(UHD_PIMPL_MAKE(impl, (start, stop, step)))
    {
        if (stop < start){
            throw std::invalid_argument("cannot make range where stop < start");
        }
    }

    template <typename T> const T range_t<T>::start(void) const{
        return _impl->start;
    }

    template <typename T> const T range_t<T>::stop(void) const{
        return _impl->stop;
    }

    template <typename T> const T range_t<T>::step(void) const{
        return _impl->step;
    }

    /*******************************************************************
     * meta_range_t implementation code
     ******************************************************************/

    template <typename T> meta_range_t<T>::meta_range_t(void){
        /* NOP */
    }

    template <typename T> template <typename InputIterator>
    meta_range_t<T>::meta_range_t(
        InputIterator first, InputIterator last
    ):
        std::vector<range_t<T> >(first, last)
    {
        /* NOP */
    }

    template <typename T> meta_range_t<T>::meta_range_t(
        const T &start, const T &stop, const T &step
    ):
        std::vector<range_t<T> > (1, range_t<T>(start, stop, step))
    {
        /* NOP */
    }

    template <typename T> const T meta_range_t<T>::start(void) const{
        if (this->empty()){
            throw std::runtime_error("cannot calculate overall start on empty meta-range");
        }
        T min_start = this->at(0).start();
        for (size_t i = 1; i < this->size(); i++){
            min_start = std::min(min_start, this->at(i).start());
        }
        return min_start;
    }

    template <typename T> const T meta_range_t<T>::stop(void) const{
        if (this->empty()){
            throw std::runtime_error("cannot calculate overall stop on empty meta-range");
        }
        T max_stop = this->at(0).stop();
        for (size_t i = 1; i < this->size(); i++){
            max_stop = std::max(max_stop, this->at(i).stop());
        }
        return max_stop;
    }

    template <typename T> const T meta_range_t<T>::step(void) const{
        if (this->empty()){
            throw std::runtime_error("cannot calculate overall step on empty meta-range");
        }
        T min_step  = this->at(0).step();
        for (size_t i = 1; i < this->size(); i++){
            if (this->at(i).start() < this->at(i-1).stop()){
                throw std::runtime_error("cannot calculate overall range when start(n) < stop(n-1) ");
            }
            if (this->at(i).start() != this->at(i).stop()){
                min_step = std::min(min_step, this->at(i).step());
            }
            min_step = std::min(min_step, this->at(i).start() - this->at(i-1).stop());
        }
        return min_step;
    }

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_RANGES_IPP */

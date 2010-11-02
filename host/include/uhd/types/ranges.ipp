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
#include <boost/foreach.hpp>
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

    namespace /*anon*/{
        template <typename T> inline
        void check_meta_range_monotonic(const meta_range_t<T> &mr){
            if (mr.empty()){
                throw std::runtime_error("meta-range cannot be empty");
            }
            for (size_t i = 1; i < mr.size(); i++){
                if (mr.at(i).start() < mr.at(i-1).stop()){
                    throw std::runtime_error("meta-range is not monotonic");
                }
            }
        }
    } //namespace /*anon*/


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
        check_meta_range_monotonic(*this);
        T min_start = this->front().start();
        BOOST_FOREACH(const range_t<T> &r, (*this)){
            min_start = std::min(min_start, r.start());
        }
        return min_start;
    }

    template <typename T> const T meta_range_t<T>::stop(void) const{
        check_meta_range_monotonic(*this);
        T max_stop = this->front().stop();
        BOOST_FOREACH(const range_t<T> &r, (*this)){
            max_stop = std::max(max_stop, r.stop());
        }
        return max_stop;
    }

    template <typename T> const T meta_range_t<T>::step(void) const{
        check_meta_range_monotonic(*this);
        std::vector<T> non_zero_steps;
        range_t<T> last = this->front();
        BOOST_FOREACH(const range_t<T> &r, (*this)){
            //steps at each range
            if (r.step() != T(0)) non_zero_steps.push_back(r.step());
            //and steps in-between ranges
            T ibtw_step = r.start() - last.stop();
            if (ibtw_step != T(0)) non_zero_steps.push_back(ibtw_step);
            //store ref to last
            last = r;
        }
        if (non_zero_steps.empty()) return T(0); //all zero steps, its zero...
        return *std::min_element(non_zero_steps.begin(), non_zero_steps.end());
    }

    template <typename T> const T meta_range_t<T>::clip(
        const T &value, bool clip_step
    ) const{
        check_meta_range_monotonic(*this);
        T last_stop = this->front().stop();
        BOOST_FOREACH(const range_t<T> &r, (*this)){
            //in-between ranges, clip to nearest
            if (value < r.start()){
                return (std::abs(value - r.start()) < std::abs(value - last_stop))?
                    r.start() : last_stop;
            }
            //in this range, clip here
            if (value <= r.stop()){
                if (not clip_step or r.step() == T(0)) return value;
                return boost::math::round((value - r.start())/r.step())*r.step() + r.start();
            }
            //continue on to the next range
            last_stop = r.stop();
        }
        return last_stop;
    }

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_RANGES_IPP */

//
// Copyright 2014 Ettus Research LLC
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

#include <uhd/utils/msg.hpp>
#include <cmath>
#include <typeinfo>

#ifndef INCLUDED_UHD_UTILS_FLOAT_COMPARE_DELTA_IPP
#define INCLUDED_UHD_UTILS_FLOAT_COMPARE_DELTA_IPP


namespace uhd { namespace math { namespace fp_compare {

    template<typename float_t> UHD_INLINE
    float_t fp_compare_select_delta(float_t lhs_delta, float_t rhs_delta) {
        return ((lhs_delta > rhs_delta) ? lhs_delta : rhs_delta);
    }

    template<> UHD_INLINE
    fp_compare_delta<float>::fp_compare_delta(float value) {

        _value = value;
        _delta = SINGLE_PRECISION_DELTA;
    }

    template<> UHD_INLINE
    fp_compare_delta<double>::fp_compare_delta(double value) {
        _value = value;
        _delta = DOUBLE_PRECISION_DELTA;
    }

    template<typename float_t> UHD_INLINE
    fp_compare_delta<float_t>::fp_compare_delta(float_t value, float_t delta)
        :   _value(value),
            _delta(delta)
    { /* NOP */ }

    template<typename float_t> UHD_INLINE
    fp_compare_delta<float_t>::fp_compare_delta(const fp_compare_delta<float_t>& copy)
        :   _value(copy._value),
            _delta(copy._delta)
    { /* NOP */ }

    template<typename float_t> UHD_INLINE
    fp_compare_delta<float_t>::~fp_compare_delta()
    { /* NOP */ }

    template<typename float_t> UHD_INLINE
    void fp_compare_delta<float_t>::operator=(const fp_compare_delta<float_t>& copy) {
        _value = copy._value;
        _delta = copy._delta;
    }

    template<typename float_t> UHD_INLINE
    bool operator==(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs) {
        float_t delta = fp_compare_select_delta(lhs._delta, rhs._delta);
        return (std::fabs(lhs._value - rhs._value) < delta);
    }

    template<typename float_t> UHD_INLINE
    bool operator!=(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs) {
        return !(lhs == rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator<(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs) {
        float_t delta = fp_compare_select_delta(lhs._delta, rhs._delta);
        return ((rhs._value - lhs._value) > delta);
    }

    template<typename float_t> UHD_INLINE
    bool operator<=(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs) {
        return !(lhs > rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator>(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs) {
        float_t delta = fp_compare_select_delta(lhs._delta, rhs._delta);
        return ((lhs._value - rhs._value) > delta);
    }

    template<typename float_t> UHD_INLINE
    bool operator>=(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs) {
        return !(lhs < rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator==(fp_compare_delta<float_t> lhs, double rhs) {
        float_t delta = float_t(fp_compare_select_delta(double(lhs._delta),
                DOUBLE_PRECISION_DELTA));
        return (std::fabs(lhs._value - rhs) < delta);
    }

    template<typename float_t> UHD_INLINE
    bool operator!=(fp_compare_delta<float_t> lhs, double rhs) {
        return !(lhs == rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator<(fp_compare_delta<float_t> lhs, double rhs) {
        float_t delta = float_t(fp_compare_select_delta(double(lhs._delta),
                DOUBLE_PRECISION_DELTA));
        return ((rhs - lhs._value) > delta);
    }

    template<typename float_t> UHD_INLINE
    bool operator<=(fp_compare_delta<float_t> lhs, double rhs) {
        return !(lhs > rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator>(fp_compare_delta<float_t> lhs, double rhs) {
        float_t delta = float_t(fp_compare_select_delta(double(lhs._delta),
                DOUBLE_PRECISION_DELTA));
        return ((lhs._value - rhs) > delta);
    }

    template<typename float_t> UHD_INLINE
    bool operator>=(fp_compare_delta<float_t> lhs, double rhs) {
        return !(lhs < rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator==(double lhs, fp_compare_delta<float_t> rhs) {
        float_t delta = fp_compare_select_delta(DOUBLE_PRECISION_DELTA,
                double(rhs._delta));
        return (std::fabs(lhs - rhs._value) < delta);
    }

    template<typename float_t> UHD_INLINE
    bool operator!=(double lhs, fp_compare_delta<float_t> rhs) {
        return !(lhs == rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator<(double lhs, fp_compare_delta<float_t> rhs) {
        float_t delta = float_t(fp_compare_select_delta(DOUBLE_PRECISION_DELTA,
                double(rhs._delta)));
        return ((rhs._value - lhs) > delta);
    }

    template<typename float_t> UHD_INLINE
    bool operator<=(double lhs, fp_compare_delta<float_t> rhs) {
        return !(lhs > rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator>(double lhs, fp_compare_delta<float_t> rhs) {
        float_t delta = float_t(fp_compare_select_delta(DOUBLE_PRECISION_DELTA,
                double(rhs._delta)));
        return ((lhs - rhs._value) > delta);
    }

    template<typename float_t> UHD_INLINE
    bool operator>=(double lhs, fp_compare_delta<float_t> rhs) {
        return !(lhs < rhs);
    }

} } } //namespace uhd::math::fp_compare

#endif /* INCLUDED_UHD_UTILS_FLOAT_COMPARE_DELTA_IPP */

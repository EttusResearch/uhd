//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include <cmath>
#include <typeinfo>

#ifndef INCLUDED_UHD_UTILS_FP_COMPARE_EPSILON_IPP
#define INCLUDED_UHD_UTILS_FP_COMPARE_EPSILON_IPP


namespace uhd { namespace math { namespace fp_compare {

    template<> UHD_INLINE
    fp_compare_epsilon<float>::fp_compare_epsilon(float value) {

        _value = value;
        _epsilon = SINGLE_PRECISION_EPSILON;
    }

    template<> UHD_INLINE
    fp_compare_epsilon<double>::fp_compare_epsilon(double value) {
        _value = value;
        _epsilon = DOUBLE_PRECISION_EPSILON;
    }

    template<typename float_t> UHD_INLINE
    fp_compare_epsilon<float_t>::fp_compare_epsilon(float_t value, float_t epsilon)
        :   _value(value),
            _epsilon(epsilon)
    { /* NOP */ }

    template<typename float_t> UHD_INLINE
    fp_compare_epsilon<float_t>::fp_compare_epsilon(const fp_compare_epsilon<float_t>& copy)
        :   _value(copy._value),
            _epsilon(copy._epsilon)
    { /* NOP */ }

    template<typename float_t> UHD_INLINE
    fp_compare_epsilon<float_t>::~fp_compare_epsilon()
    { /* NOP */ }

    template<typename float_t> UHD_INLINE
    void fp_compare_epsilon<float_t>::operator=(const fp_compare_epsilon<float_t>& copy) {
        _value = copy._value;
        _epsilon = copy._epsilon;
    }

    template<typename float_t> UHD_INLINE
    bool operator==(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs) {

        bool lhs_compare = ((std::fabs(lhs._value - rhs._value) / std::fabs(lhs._value))
                    <= lhs._epsilon);
        bool rhs_compare = ((std::fabs(lhs._value - rhs._value) / std::fabs(rhs._value))
                    <= rhs._epsilon);

        return (lhs_compare && rhs_compare);
    }

    template<typename float_t> UHD_INLINE
    bool operator!=(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs) {
        return !(lhs == rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator<(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs) {
        return (lhs._value + lhs._epsilon) <  (rhs._value - rhs._epsilon);
    }

    template<typename float_t> UHD_INLINE
    bool operator<=(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs) {
        return !(lhs > rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator>(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs) {
        return (lhs._value - lhs._epsilon) > (rhs._value + rhs._epsilon);
    }

    template<typename float_t> UHD_INLINE
    bool operator>=(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs) {
        return !(lhs < rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator==(fp_compare_epsilon<float_t> lhs, double rhs) {

        bool lhs_compare = ((std::fabs(lhs._value - rhs) / std::fabs(lhs._value))
                    <= lhs._epsilon);
        bool rhs_compare = ((std::fabs(lhs._value - rhs) / std::fabs(rhs))
                    <= DOUBLE_PRECISION_EPSILON);

        return (lhs_compare && rhs_compare);
    }

    template<typename float_t> UHD_INLINE
    bool operator!=(fp_compare_epsilon<float_t> lhs, double rhs) {
        return !(lhs == rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator<(fp_compare_epsilon<float_t> lhs, double rhs) {

        return (lhs._value + lhs._epsilon) <  (rhs - DOUBLE_PRECISION_EPSILON);
    }

    template<typename float_t> UHD_INLINE
    bool operator<=(fp_compare_epsilon<float_t> lhs, double rhs) {
        return !(lhs > rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator>(fp_compare_epsilon<float_t> lhs, double rhs) {

        return (lhs._value - lhs._epsilon) > (rhs + DOUBLE_PRECISION_EPSILON);
    }

    template<typename float_t> UHD_INLINE
    bool operator>=(fp_compare_epsilon<float_t> lhs, double rhs) {
        return !(lhs < rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator==(double lhs, fp_compare_epsilon<float_t> rhs) {

        bool lhs_compare = ((std::fabs(lhs - rhs._value) / std::fabs(lhs))
                    <= DOUBLE_PRECISION_EPSILON);
        bool rhs_compare = ((std::fabs(lhs - rhs._value) / std::fabs(rhs._value))
                    <= rhs._epsilon);

        return (lhs_compare && rhs_compare);
    }

    template<typename float_t> UHD_INLINE
    bool operator!=(double lhs, fp_compare_epsilon<float_t> rhs) {
        return !(lhs == rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator<(double lhs, fp_compare_epsilon<float_t> rhs) {

        return (lhs + DOUBLE_PRECISION_EPSILON) <  (rhs._value - rhs._epsilon);
    }

    template<typename float_t> UHD_INLINE
    bool operator<=(double lhs, fp_compare_epsilon<float_t> rhs) {
        return !(lhs > rhs);
    }

    template<typename float_t> UHD_INLINE
    bool operator>(double lhs, fp_compare_epsilon<float_t> rhs) {

        return (lhs - DOUBLE_PRECISION_EPSILON) > (rhs._value + rhs._epsilon);
    }

    template<typename float_t> UHD_INLINE
    bool operator>=(double lhs, fp_compare_epsilon<float_t> rhs) {
        return !(lhs < rhs);
    }

} } } //namespace uhd::math::fp_compare

#endif /* INCLUDED_UHD_UTILS_FP_COMPARE_EPSILON_IPP */

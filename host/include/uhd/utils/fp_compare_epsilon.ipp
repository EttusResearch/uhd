//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include <cmath>
#include <typeinfo>


#pragma once


namespace uhd { namespace math { namespace fp_compare {

template <>
UHD_INLINE fp_compare_epsilon<float>::fp_compare_epsilon(float value)
{
    _value   = value;
    _epsilon = SINGLE_PRECISION_EPSILON;
}

template <>
UHD_INLINE fp_compare_epsilon<double>::fp_compare_epsilon(double value)
{
    _value   = value;
    _epsilon = DOUBLE_PRECISION_EPSILON;
}

template <typename float_t>
UHD_INLINE fp_compare_epsilon<float_t>::fp_compare_epsilon(float_t value, float_t epsilon)
    : _value(value), _epsilon(epsilon)
{ /* NOP */
}

template <typename float_t>
UHD_INLINE fp_compare_epsilon<float_t>::fp_compare_epsilon(
    const fp_compare_epsilon<float_t>& copy)
    : _value(copy._value), _epsilon(copy._epsilon)
{ /* NOP */
}

template <typename float_t>
UHD_INLINE fp_compare_epsilon<float_t>::~fp_compare_epsilon()
{ /* NOP */
}

template <typename float_t>
UHD_INLINE void fp_compare_epsilon<float_t>::operator=(
    const fp_compare_epsilon<float_t>& copy)
{
    _value   = copy._value;
    _epsilon = copy._epsilon;
}

template <typename float_t>
UHD_INLINE bool operator==(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs)
{
    // If raw bit values are equal, then they're equal. This also catches
    // the case where both are 0.0!
    if (lhs._value == rhs._value) {
        return true;
    }

    // If one of them is within epsilon of zero, but the other is not, then
    // they're also not equal.
    if ((std::abs(lhs._value) <= lhs._epsilon && std::abs(rhs._value) > rhs._epsilon)
        || (std::abs(lhs._value) > lhs._epsilon
            && std::abs(rhs._value) <= rhs._epsilon)) {
        return false;
    }

    // In all other cases, we use the "close enough with tolerance epsilon"
    // algorithm as described in math.hpp.
    const bool lhs_compare =
        ((std::abs(lhs._value - rhs._value) / std::abs(lhs._value)) <= lhs._epsilon);
    const bool rhs_compare =
        ((std::abs(lhs._value - rhs._value) / std::abs(rhs._value)) <= rhs._epsilon);

    return (lhs_compare || rhs_compare);
}

template <typename float_t>
UHD_INLINE bool operator!=(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs)
{
    return !(lhs == rhs);
}

template <typename float_t>
UHD_INLINE bool operator<(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs)
{
    return (lhs != rhs) && (lhs._value < rhs._value);
}

template <typename float_t>
UHD_INLINE bool operator<=(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs)
{
    return !(lhs > rhs);
}

template <typename float_t>
UHD_INLINE bool operator>(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs)
{
    return (lhs != rhs) && (lhs._value > rhs._value);
}

template <typename float_t>
UHD_INLINE bool operator>=(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs)
{
    return !(lhs < rhs);
}

template <typename float_t>
UHD_INLINE bool operator==(fp_compare_epsilon<float_t> lhs, double rhs)
{
    return lhs == fp_compare_epsilon<float_t>(static_cast<float_t>(rhs));
}

template <typename float_t>
UHD_INLINE bool operator!=(fp_compare_epsilon<float_t> lhs, double rhs)
{
    return !(lhs == rhs);
}

template <typename float_t>
UHD_INLINE bool operator<(fp_compare_epsilon<float_t> lhs, double rhs)
{
    return (lhs != rhs) && (lhs._value < rhs);
}

template <typename float_t>
UHD_INLINE bool operator<=(fp_compare_epsilon<float_t> lhs, double rhs)
{
    return !(lhs > rhs);
}

template <typename float_t>
UHD_INLINE bool operator>(fp_compare_epsilon<float_t> lhs, double rhs)
{
    return (lhs != rhs) && (lhs._value > rhs);
}

template <typename float_t>
UHD_INLINE bool operator>=(fp_compare_epsilon<float_t> lhs, double rhs)
{
    return !(lhs < rhs);
}

template <typename float_t>
UHD_INLINE bool operator==(double lhs, fp_compare_epsilon<float_t> rhs)
{
    return fp_compare_epsilon<float_t>(static_cast<float_t>(lhs)) == rhs;
}

template <typename float_t>
UHD_INLINE bool operator!=(double lhs, fp_compare_epsilon<float_t> rhs)
{
    return !(lhs == rhs);
}

template <typename float_t>
UHD_INLINE bool operator<(double lhs, fp_compare_epsilon<float_t> rhs)
{
    return (lhs != rhs) && (lhs < rhs._value);
}

template <typename float_t>
UHD_INLINE bool operator<=(double lhs, fp_compare_epsilon<float_t> rhs)
{
    return !(lhs > rhs);
}

template <typename float_t>
UHD_INLINE bool operator>(double lhs, fp_compare_epsilon<float_t> rhs)
{
    return (lhs != rhs) && (lhs > rhs._value);
}

template <typename float_t>
UHD_INLINE bool operator>=(double lhs, fp_compare_epsilon<float_t> rhs)
{
    return !(lhs < rhs);
}

}}} // namespace uhd::math::fp_compare

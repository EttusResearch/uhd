//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <cmath>


namespace uhd {

/*!
 * Contains useful mathematical functions, classes, and constants, which should
 * be used in UHD when portable / `std` options are not available.
 */
namespace math {

static const double PI = 3.14159265358979323846;

/*!
 * Define epsilon values for floating point comparisons.
 *
 * There are a lot of different sources for epsilon values that we could use
 * for this. For single-precision (f32), most machines will report an
 * epsilon of 1.192e-7, and for double-precision (f64) most machines will
 * report an epsilon of 2.220e-16. The issue is that these are not always
 * appropriate, depending on the scale of the operands and how they have
 * been rounded in previous calculations. The values defined here are
 * defaults, but should be overridden for calculations depending on the
 * application.
 *
 * If a particular comparison is operating using very small or very large
 * values, a custom epsilon should be defined for those computations. This
 * use-case is provided for in the `fp_compare_epsilon` class constructor.
 */
static const float SINGLE_PRECISION_EPSILON  = 1.19e-7f;
static const double DOUBLE_PRECISION_EPSILON = 2.22e-16;

//! Epsilon value for when using fp_compare_epsilon to compare frequencies
//
// Note that this is a UHD/USRP-specific constant. In UHD, frequencies are on
// the order of 1e9 (GHz) or below. We will declare frequencies as equal if they
// are within a millihertz.  For the purpose of
// comparing frequencies, we "lose" 9 decimal places for the integer
// component of the frequency, so we choose this epsilon value for floating
// point comparison of frequencies.
static constexpr double FREQ_COMPARE_EPSILON = 1e-12;

namespace fp_compare {

/*!
 * Class for floating-point comparisons using an epsilon.
 *
 * At construction, you can specify the epsilon to use for the comparisons.
 * This class, combined with the operators under it, allow for
 * epsilon-comparisons of floats. An example is:
 *
 * // Compare floats 'x' and 'y'.
 * bool x_equals_y = (fp_compare_epsilon<float>(x) == y);
 *
 * // Compare doubles 'x' and 'y'.
 * bool x_equals_y = (fp_compare_epsilon<double>(x) == y);
 */
template <typename float_t>
class fp_compare_epsilon
{
public:
    UHD_INLINE fp_compare_epsilon(float_t value);
    UHD_INLINE fp_compare_epsilon(float_t value, float_t epsilon);
    UHD_INLINE fp_compare_epsilon(const fp_compare_epsilon<float_t>& copy);
    UHD_INLINE ~fp_compare_epsilon();
    UHD_INLINE void operator=(const fp_compare_epsilon& copy);

    float_t _value;
    float_t _epsilon;
};

/* A Note on Floating Point Equality with Epsilons
 *
 * There are obviously a lot of strategies for defining floating point
 * equality, and in the end it all comes down to the domain at hand. UHD's
 * floating-point-with-epsilon comparison algorithm is based on the method
 * presented in Knuth's "The Art of Computer Science" called "close enough
 * with tolerance epsilon".
 *
 *      [(|u - v| / |u|) <= e] || [(|u - v| / |v|) <= e]
 *
 * UHD's modification to this algorithm is using the denominator's epsilon
 * value (since each float_t object has its own epsilon) for each
 * comparison.
 */

template <typename float_t>
UHD_INLINE bool operator==(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator!=(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator<(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator<=(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator>(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator>=(
    fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);

/* If these operators are used with floats, we rely on type promotion to
 * double. */
template <typename float_t>
UHD_INLINE bool operator==(fp_compare_epsilon<float_t> lhs, double rhs);
template <typename float_t>
UHD_INLINE bool operator!=(fp_compare_epsilon<float_t> lhs, double rhs);
template <typename float_t>
UHD_INLINE bool operator<(fp_compare_epsilon<float_t> lhs, double rhs);
template <typename float_t>
UHD_INLINE bool operator<=(fp_compare_epsilon<float_t> lhs, double rhs);
template <typename float_t>
UHD_INLINE bool operator>(fp_compare_epsilon<float_t> lhs, double rhs);
template <typename float_t>
UHD_INLINE bool operator>=(fp_compare_epsilon<float_t> lhs, double rhs);

template <typename float_t>
UHD_INLINE bool operator==(double lhs, fp_compare_epsilon<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator!=(double lhs, fp_compare_epsilon<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator<(double lhs, fp_compare_epsilon<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator<=(double lhs, fp_compare_epsilon<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator>(double lhs, fp_compare_epsilon<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator>=(double lhs, fp_compare_epsilon<float_t> rhs);

//! An alias for fp_compare_epsilon, but with defaults for frequencies
class freq_compare_epsilon : public fp_compare_epsilon<double>
{
public:
    UHD_INLINE freq_compare_epsilon(double value)
        : fp_compare_epsilon<double>(value, FREQ_COMPARE_EPSILON)
    {
    }

    UHD_INLINE freq_compare_epsilon(const freq_compare_epsilon& copy)
        : fp_compare_epsilon<double>(copy)
    {
    }
};


} // namespace fp_compare


/*!
 * Define delta values for floating point comparisons.
 *
 * These are the default deltas used by the 'fp_compare_delta' class for
 * single and double-precision floating point comparisons.
 */
static const float SINGLE_PRECISION_DELTA  = 1e-3f;
static const double DOUBLE_PRECISION_DELTA = 1e-5;

/*! Floating-point delta to use for frequency comparisons. */
static const double FREQ_COMPARISON_DELTA_HZ = 0.1;


namespace fp_compare {

/*!
 * Class for floating-point comparisons using a delta.
 *
 * At construction, you can specify the delta to use for the comparisons.
 * This class, combined with the operators under it, allow for
 * delta-comparisons of floats. An example is:
 *
 * // Compare floats 'x' and 'y'.
 * bool x_equals_y = (fp_compare_delta<float>(x) == y);
 *
 * // Compare doubles 'x' and 'y'.
 * bool x_equals_y = (fp_compare_delta<double>(x) == y);
 */
template <typename float_t>
class fp_compare_delta
{
public:
    UHD_INLINE fp_compare_delta(float_t value);
    UHD_INLINE fp_compare_delta(float_t value, float_t delta);
    UHD_INLINE fp_compare_delta(const fp_compare_delta<float_t>& copy);
    UHD_INLINE ~fp_compare_delta();
    UHD_INLINE void operator=(const fp_compare_delta& copy);

    float_t _value;
    float_t _delta;
};

template <typename float_t>
UHD_INLINE bool operator==(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator!=(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator<(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator<=(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator>(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator>=(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);

/* If these operators are used with floats, we rely on type promotion to
 * double. */
template <typename float_t>
UHD_INLINE bool operator==(fp_compare_delta<float_t> lhs, double rhs);
template <typename float_t>
UHD_INLINE bool operator!=(fp_compare_delta<float_t> lhs, double rhs);
template <typename float_t>
UHD_INLINE bool operator<(fp_compare_delta<float_t> lhs, double rhs);
template <typename float_t>
UHD_INLINE bool operator<=(fp_compare_delta<float_t> lhs, double rhs);
template <typename float_t>
UHD_INLINE bool operator>(fp_compare_delta<float_t> lhs, double rhs);
template <typename float_t>
UHD_INLINE bool operator>=(fp_compare_delta<float_t> lhs, double rhs);

template <typename float_t>
UHD_INLINE bool operator==(double lhs, fp_compare_delta<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator!=(double lhs, fp_compare_delta<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator<(double lhs, fp_compare_delta<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator<=(double lhs, fp_compare_delta<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator>(double lhs, fp_compare_delta<float_t> rhs);
template <typename float_t>
UHD_INLINE bool operator>=(double lhs, fp_compare_delta<float_t> rhs);

} // namespace fp_compare

UHD_INLINE bool frequencies_are_equal(double lhs, double rhs)
{
    return (fp_compare::fp_compare_delta<double>(lhs, FREQ_COMPARISON_DELTA_HZ)
            == fp_compare::fp_compare_delta<double>(rhs, FREQ_COMPARISON_DELTA_HZ));
}

inline double dB_to_lin(const double dB_val)
{
    return std::pow(10, (dB_val) / 10.0);
}

inline double lin_to_dB(const double val)
{
    return 10 * std::log10(val);
}


//! Returns the sign of x
//
// Note: This is equivalent to the Boost.Math version, but without the
// dependency.
//
// Returns +1 for positive arguments, -1 for negative arguments, and 0 if x is
// zero.
template <typename T>
inline constexpr int sign(T x)
{
    // Note: If T is unsigned, then this will compile with a warning. Should
    // we need that, expand the template logic.
    return (T(0) < x) - (x < T(0));
}

//! Return a wrapped frequency that is the equivalent frequency in the first
// Nyquist zone.
//
// Examples:
// - Just above the sampling rate:
//   wrap_frequency(250e6, 200e6) == 50e6
// - Just outside the Nyquist zone:
//   wrap_frequency(120e6, 200e6) == -80e6
// - Also works for negative frequencies:
//   wrap_frequency(-250e6, 200e6) == -50e6
inline double wrap_frequency(const double requested_freq, const double rate)
{
    double freq = std::fmod(requested_freq, rate);
    if (std::abs(freq) > rate / 2.0)
        freq -= uhd::math::sign(freq) * rate;
    return freq;
}

} // namespace math
} // namespace uhd

#include <uhd/utils/fp_compare_delta.ipp>
#include <uhd/utils/fp_compare_epsilon.ipp>

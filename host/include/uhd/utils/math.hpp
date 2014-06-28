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

#ifndef INCLUDED_UHD_UTILS_MATH_HPP
#define INCLUDED_UHD_UTILS_MATH_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>
#include <boost/numeric/conversion/bounds.hpp>


namespace uhd {

/*!
 * Contains useful mathematical functions, classes, and constants, which should
 * be used in UHD when portable / `std` options are not available.
 */
namespace math {

    /*!
     * Numeric limits of certain types.
     *
     * There are many sources for getting these, including std::numeric_limts,
     * <cstdint>, <climits>, and Boost. The <cstdint> option is preferable as it
     * gives us fixed-width constants, but unfortunately is new as of C++11.
     * Since this isn't available on many systems, we need to use one of the
     * other options. We will use the Boost option, here, since we use Boost
     * data types for portability across UHD.
     */
    static const boost::int32_t BOOST_INT32_MAX = boost::numeric::bounds<boost::int32_t>::highest();
    static const boost::int32_t BOOST_INT32_MIN = boost::numeric::bounds<boost::int32_t>::lowest();

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
    static const float SINGLE_PRECISION_EPSILON = 1.19e-7;
    static const double DOUBLE_PRECISION_EPSILON = 2.22e-16;

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
    template<typename float_t> class fp_compare_epsilon {
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
     * presented in Knuth's "The Art of Computer Science" called "very close
     * with tolerance epsilon".
     *
     *      [(|u - v| / |u|) <= e] && [(|u - v| / |v|) <= e]
     *
     * UHD's modification to this algorithm is using the denominator's epsilon
     * value (since each float_t object has its own epsilon) for each
     * comparison.
     */

    template<typename float_t> UHD_INLINE
        bool operator==(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator!=(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator<(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator<=(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator>(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator>=(fp_compare_epsilon<float_t> lhs, fp_compare_epsilon<float_t> rhs);

    /* If these operators are used with floats, we rely on type promotion to
     * double. */
    template<typename float_t> UHD_INLINE
        bool operator==(fp_compare_epsilon<float_t> lhs, double rhs);
    template<typename float_t> UHD_INLINE
        bool operator!=(fp_compare_epsilon<float_t> lhs, double rhs);
    template<typename float_t> UHD_INLINE
        bool operator<(fp_compare_epsilon<float_t> lhs, double rhs);
    template<typename float_t> UHD_INLINE
        bool operator<=(fp_compare_epsilon<float_t> lhs, double rhs);
    template<typename float_t> UHD_INLINE
        bool operator>(fp_compare_epsilon<float_t> lhs, double rhs);
    template<typename float_t> UHD_INLINE
        bool operator>=(fp_compare_epsilon<float_t> lhs, double rhs);

    template<typename float_t> UHD_INLINE
        bool operator==(double lhs, fp_compare_epsilon<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator!=(double lhs, fp_compare_epsilon<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator<(double lhs, fp_compare_epsilon<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator<=(double lhs, fp_compare_epsilon<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator>(double lhs, fp_compare_epsilon<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator>=(double lhs, fp_compare_epsilon<float_t> rhs);

} // namespace fp_compare


    /*!
     * Define delta values for floating point comparisons.
     *
     * These are the default deltas used by the 'fp_compare_delta' class for
     * single and double-precision floating point comparisons.
     */
    static const float SINGLE_PRECISION_DELTA = 1e-3;
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
    template<typename float_t> class fp_compare_delta {
        public:
            UHD_INLINE fp_compare_delta(float_t value);
            UHD_INLINE fp_compare_delta(float_t value, float_t delta);
            UHD_INLINE fp_compare_delta(const fp_compare_delta<float_t>& copy);
            UHD_INLINE ~fp_compare_delta();
            UHD_INLINE void operator=(const fp_compare_delta& copy);

            float_t _value;
            float_t _delta;
    };

    template<typename float_t> UHD_INLINE
        bool operator==(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator!=(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator<(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator<=(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator>(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator>=(fp_compare_delta<float_t> lhs, fp_compare_delta<float_t> rhs);

    /* If these operators are used with floats, we rely on type promotion to
     * double. */
    template<typename float_t> UHD_INLINE
        bool operator==(fp_compare_delta<float_t> lhs, double rhs);
    template<typename float_t> UHD_INLINE
        bool operator!=(fp_compare_delta<float_t> lhs, double rhs);
    template<typename float_t> UHD_INLINE
        bool operator<(fp_compare_delta<float_t> lhs, double rhs);
    template<typename float_t> UHD_INLINE
        bool operator<=(fp_compare_delta<float_t> lhs, double rhs);
    template<typename float_t> UHD_INLINE
        bool operator>(fp_compare_delta<float_t> lhs, double rhs);
    template<typename float_t> UHD_INLINE
        bool operator>=(fp_compare_delta<float_t> lhs, double rhs);

    template<typename float_t> UHD_INLINE
        bool operator==(double lhs, fp_compare_delta<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator!=(double lhs, fp_compare_delta<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator<(double lhs, fp_compare_delta<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator<=(double lhs, fp_compare_delta<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator>(double lhs, fp_compare_delta<float_t> rhs);
    template<typename float_t> UHD_INLINE
        bool operator>=(double lhs, fp_compare_delta<float_t> rhs);

} // namespace fp_compare

    UHD_INLINE bool frequencies_are_equal(double lhs, double rhs) {
        return(fp_compare::fp_compare_delta<double>(lhs, FREQ_COMPARISON_DELTA_HZ)
                == fp_compare::fp_compare_delta<double>(rhs, FREQ_COMPARISON_DELTA_HZ));
    }

} // namespace math
} // namespace uhd

#include "fp_compare_epsilon.ipp"
#include "fp_compare_delta.ipp"

#endif /* INCLUDED_UHD_UTILS_MATH_HPP */

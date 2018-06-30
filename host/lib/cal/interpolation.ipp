//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_INTERPOLATION_IPP
#define INCLUDED_UHD_INTERPOLATION_IPP

#include "interpolation.hpp"
#include <uhd/utils/log.hpp>
// This is a bugfix for Boost 1.64, maybe future Boosts won't need this
#if BOOST_VERSION >= 106400
#  include <boost/serialization/array_wrapper.hpp>
#endif // end of bugfix
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>

using namespace boost::numeric;

namespace uhd {
namespace cal {

#define CAL_INTERP_METHOD(return_type, method, args, ...) \
    template<typename in_type, typename out_type> \
    return_type interp<in_type, out_type>::\
    method(args, __VA_ARGS__)

#define ARGS_T typename interp<in_type, out_type>::args_t
#define CONTAINER_T typename interp<in_type, out_type>::container_t

CAL_INTERP_METHOD(in_type, calc_dist, const ARGS_T &a, const ARGS_T &b)
{
    in_type dist = 0;
    for (size_t i = 0; i < std::min(a.size(), b.size()); i++)
    {
        dist += std::abs(a[i] - b[i]);
    }
    return dist;
}

CAL_INTERP_METHOD(const out_type, nn_interp, CONTAINER_T &data, const ARGS_T &args)
{
    // Check the cache for the output
    if (data.find(args) != data.end()) {
        return data[args];
    }

    out_type output = 0;
    in_type min_dist = 0;
    typename container_t::const_iterator citer;
    for (citer = data.begin(); citer != data.end(); citer++)
    {
        in_type dist = calc_dist(citer->first, args);
        if (citer == data.begin() || dist < min_dist) {
            min_dist = dist;
            output = data[citer->first];
        }
    }

    return output;
}

CAL_INTERP_METHOD(const out_type, bl_interp, CONTAINER_T &data, const ARGS_T &args)
{
    if (args.size() != 2) {
        throw uhd::assertion_error(str(boost::format(
            "Bilinear interpolation expects 2D values. Received %d.")
            % args.size()
        ));
    }

    if (data.size() < 4) {
        throw uhd::assertion_error(str(boost::format(
            "Bilinear interpolation requires at least 4 input points. Found %d.")
            % data.size()
        ));
    }

    // Locate the nearest 4 points
    typedef std::pair<interp<in_type, out_type>::args_t, out_type> cal_pair_t;
    typename std::vector<cal_pair_t> nearest;

    // Initialize the resulting pair to something
    cal_pair_t pair = *data.begin();

    for (size_t i = 0; i < 4; i++) {
        bool init = true;
        in_type min_dist = 0;
        typename container_t::const_iterator citer;
        for (citer = data.begin(); citer != data.end(); citer++)
        {
            cal_pair_t temp = *citer;
            if (std::find(nearest.begin(), nearest.end(), temp) == nearest.end())
            {
                in_type dist = calc_dist(citer->first, args);
                if (dist < min_dist || init)
                {
                    min_dist = dist;
                    pair = temp;
                    init = false;
                }
            }
        }
        // Push back the nearest pair
        nearest.push_back(pair);
    }

    //
    // Since these points are not grid aligned,
    // we perform irregular bilinear interpolation.
    // This math involves finding our interpolation
    // function using lagrange multipliers:
    //
    // f(x, y) = ax^2 + bxy + cy^2 + dx + ey + f
    //
    // The solution is to solve the following system:
    //
    //    A       x       b
    // | E X' | | s | - | 0 |
    // | X 0  | | l | - | z |
    //
    // where s is a vector of the above coefficients.
    //
    typename ublas::matrix<in_type> A(10, 10, 0.0);

    // E
    A(0, 0) = 1.0; A(1, 1) = 1.0; A(2, 2) = 1.0;

    in_type x1, x2, x3, x4;
    in_type y1, y2, y3, y4;

    x1 = nearest[0].first[0]; y1 = nearest[0].first[1];
    x2 = nearest[1].first[0]; y2 = nearest[1].first[1];
    x3 = nearest[2].first[0]; y3 = nearest[2].first[1];
    x4 = nearest[3].first[0]; y4 = nearest[3].first[1];

    // X
    A(0, 6) = x1*x1; A(1, 6) = x1*y1; A(2, 6) = y1*y1; A(3, 6) = x1; A(4, 6) = y1; A(5, 6) = 1.0;
    A(0, 7) = x2*x2; A(1, 7) = x2*y2; A(2, 7) = y2*y2; A(3, 7) = x2; A(4, 7) = y2; A(5, 7) = 1.0;
    A(0, 8) = x3*x3; A(1, 8) = x3*y3; A(2, 8) = y3*y3; A(3, 8) = x3; A(4, 8) = y3; A(5, 8) = 1.0;
    A(0, 9) = x4*x4; A(1, 9) = x4*y4; A(2, 9) = y4*y4; A(3, 9) = x4; A(4, 9) = y4; A(5, 9) = 1.0;

    // X'
    A(6, 0) = x1*x1; A(6, 1) = x1*y1; A(6, 2) = y1*y1; A(6, 3) = x1; A(6, 4) = y1; A(6, 5) = 1.0;
    A(7, 0) = x2*x2; A(7, 1) = x2*y2; A(7, 2) = y2*y2; A(7, 3) = x2; A(7, 4) = y2; A(7, 5) = 1.0;
    A(8, 0) = x3*x3; A(8, 1) = x3*y3; A(8, 2) = y3*y3; A(8, 3) = x3; A(8, 4) = y3; A(8, 5) = 1.0;
    A(9, 0) = x4*x4; A(9, 1) = x4*y4; A(9, 2) = y4*y4; A(9, 3) = x4; A(9, 4) = y4; A(9, 5) = 1.0;

    // z
    typename ublas::vector<in_type> b(10, 0.0);
    b(6) = nearest[0].second;
    b(7) = nearest[1].second;
    b(8) = nearest[2].second;
    b(9) = nearest[3].second;

    typename ublas::matrix<in_type> A_t = A;
    typename ublas::vector<in_type> s = b;
    typename ublas::permutation_matrix<in_type> P(A_t.size1());

    // Use LUP factorization to solve for the coefficients
    // We're solving the problem in the form of Ax = b
    bool is_singular = ublas::lu_factorize(A_t, P);

    out_type output = 0;

    // Fall back to 1D interpolation if the matrix is singular
    if (is_singular) {
        // Warn the user that the A matrix is singular
        UHD_LOGGER_WARNING("CAL") << "Bilinear interpolation: singular matrix detected. "
                         << "Performing 1D linear interpolation against the nearest measurements. "
                         << "Provide calibration data with more measurements";

        output = (b[7] - b[6]) / 2.0;
        output += b[6];
        return output;
    }
    ublas::lu_substitute(A_t, P, s);

    in_type x = args[0];
    in_type y = args[1];

    // Utilize the solution to calculate the interpolation function
    output = s[0]*x*x + s[1]*x*y + s[2]*y*y + s[3]*x + s[4]*y + s[5];
    return output;
}

} // namespace cal
} // namespace uhd

#endif  /* INCLUDED_UHD_INTERPOLATION_IPP */

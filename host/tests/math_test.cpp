//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/math.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_lcm)
{
    BOOST_CHECK_EQUAL(uhd::math::lcm<int>(2, 3), 6);
}

BOOST_AUTO_TEST_CASE(test_gcd)
{
    BOOST_CHECK_EQUAL(uhd::math::gcd<int>(6, 15), 3);
}

BOOST_AUTO_TEST_CASE(test_interp)
{
    const double x0 = 1.0, x1 = 2.0;
    const double y0 = 2.0, y1 = 4.0;
    const double x     = 1.5;
    const double y_exp = 3.0;

    BOOST_CHECK_EQUAL(uhd::math::linear_interp<double>(x, x0, y0, x1, y1), y_exp);
}

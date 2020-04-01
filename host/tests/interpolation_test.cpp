//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/interpolation.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_interp)
{
    const double x0 = 1.0, x1 = 2.0;
    const double y0 = 2.0, y1 = 4.0;
    const double x     = 1.5;
    const double y_exp = 3.0;

    BOOST_CHECK_EQUAL(uhd::math::linear_interp<double>(x, x0, y0, x1, y1), y_exp);
}

BOOST_AUTO_TEST_CASE(test_map_interp)
{
    using namespace uhd::math;
    std::map<double, double> test_data{{1.0, 1.0}, {2.0, 2.0}, {3.0, 3.0}};

    BOOST_CHECK_EQUAL(at_nearest(test_data, 1.1), 1.0);
    BOOST_CHECK_EQUAL(at_nearest(test_data, 1.9), 2.0);
    BOOST_CHECK_EQUAL(at_nearest(test_data, 2.1), 2.0);
    BOOST_CHECK_EQUAL(at_nearest(test_data, 1e9), 3.0);

    BOOST_CHECK_CLOSE(at_lin_interp(test_data, 1.5), 1.5, 1e-6);
    BOOST_CHECK_EQUAL(at_lin_interp(test_data, 0.1), 1.0);
    BOOST_CHECK_EQUAL(at_lin_interp(test_data, 2.0), 2.0);
    BOOST_CHECK_EQUAL(at_lin_interp(test_data, 137.0), 3.0);
}


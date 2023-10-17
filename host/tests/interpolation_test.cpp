//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/interpolation.hpp>
#include <boost/test/unit_test.hpp>
#include <string>

BOOST_AUTO_TEST_CASE(test_get_bounding_iterators)
{
    using std::string;
    using namespace uhd::math;

    const std::map<double, string> data{{1.0, "leviathan wakes"}, {2.0, "calibans war"}};

    const auto test1 = get_bounding_iterators(data, 1.1);
    BOOST_CHECK_EQUAL(test1.first->first, 1.0);
    BOOST_CHECK_EQUAL(test1.second->first, 2.0);
    BOOST_CHECK_EQUAL(test1.first->second, "leviathan wakes");
    BOOST_CHECK_EQUAL(test1.second->second, "calibans war");

    const auto test2 = get_bounding_iterators(data, 0.5);
    BOOST_CHECK_EQUAL(test2.first->first, 1.0);
    BOOST_CHECK_EQUAL(test2.second->first, 1.0);
    BOOST_CHECK_EQUAL(test2.first->second, "leviathan wakes");
    BOOST_CHECK_EQUAL(test2.second->second, "leviathan wakes");

    const auto test3 = get_bounding_iterators(data, 1e6);
    BOOST_CHECK_EQUAL(test3.first->first, 2.0);
    BOOST_CHECK_EQUAL(test3.second->first, 2.0);
    BOOST_CHECK_EQUAL(test3.first->second, "calibans war");
    BOOST_CHECK_EQUAL(test3.second->second, "calibans war");

    const auto test4 = get_bounding_iterators(data, 2.0);
    BOOST_CHECK_EQUAL(test4.first->first, 1.0);
    BOOST_CHECK_EQUAL(test4.second->first, 2.0);
    BOOST_CHECK_EQUAL(test4.first->second, "leviathan wakes");
    BOOST_CHECK_EQUAL(test4.second->second, "calibans war");

    const auto test5 = get_bounding_iterators(data, 1.0);
    BOOST_CHECK_EQUAL(test5.first->first, 1.0);
    BOOST_CHECK_EQUAL(test5.second->first, 1.0);
    BOOST_CHECK_EQUAL(test5.first->second, "leviathan wakes");
    BOOST_CHECK_EQUAL(test5.second->second, "leviathan wakes");
}

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
    const std::map<double, double> test_data{{1.0, 1.0}, {2.0, 2.0}, {3.0, 3.0}};

    BOOST_CHECK_EQUAL(at_nearest(test_data, 1.1), 1.0);
    BOOST_CHECK_EQUAL(at_nearest(test_data, 1.9), 2.0);
    BOOST_CHECK_EQUAL(at_nearest(test_data, 2.1), 2.0);
    BOOST_CHECK_EQUAL(at_nearest(test_data, 1e9), 3.0);

    BOOST_CHECK_CLOSE(at_lin_interp(test_data, 1.5), 1.5, 1e-6);
    BOOST_CHECK_EQUAL(at_lin_interp(test_data, 0.1), 1.0);
    BOOST_CHECK_EQUAL(at_lin_interp(test_data, 2.0), 2.0);
    BOOST_CHECK_EQUAL(at_lin_interp(test_data, 137.0), 3.0);
}

BOOST_AUTO_TEST_CASE(test_map_bilinear_interp)
{
    using namespace uhd::math;
    // clang-format off
    std::map<double, std::map<double, double>> test_data{
        {1.0, {{1.0, 0.0}, {2.0, 1.0}}},
        {2.0, {{1.0, 1.0}, {2.0, 2.0}}}
    };
    // clang-format on

    BOOST_CHECK_CLOSE(at_bilin_interp(test_data, 1.5, 1.5), 1.0, 1e-6);
    // Move y out of bounds, keep x in bounds
    BOOST_CHECK_CLOSE(at_bilin_interp(test_data, 1.5, -17), 0.5, 1e-6);
    BOOST_CHECK_CLOSE(at_bilin_interp(test_data, 1.5, 123.4), 1.5, 1e-6);
    // Move x out of bounds, keep y in bounds
    BOOST_CHECK_CLOSE(at_bilin_interp(test_data, -1e5, 1.5), 0.5, 1e-6);
    BOOST_CHECK_CLOSE(at_bilin_interp(test_data, 1e9, 1.5), 1.5, 1e-6);
    // Move x and y out of bounds
    BOOST_CHECK_CLOSE(at_bilin_interp(test_data, -1e5, -1e6), 0.0, 1e-6);
    BOOST_CHECK_CLOSE(at_bilin_interp(test_data, -1e5, 1e6), 1.0, 1e-6);
    BOOST_CHECK_CLOSE(at_bilin_interp(test_data, 1e5, -1e6), 1.0, 1e-6);
    BOOST_CHECK_CLOSE(at_bilin_interp(test_data, 1e5, 1e6), 2.0, 1e-6);
    // Boundary conditions
    BOOST_CHECK_EQUAL(at_bilin_interp(test_data, 1.0, 1.0), 0.0);
    BOOST_CHECK_EQUAL(at_bilin_interp(test_data, 2.0, 2.0), 2.0);
    BOOST_CHECK_EQUAL(at_bilin_interp(test_data, 1.0, 1.5), 0.5);
    BOOST_CHECK_EQUAL(at_bilin_interp(test_data, 1.5, 2.0), 1.5);
}

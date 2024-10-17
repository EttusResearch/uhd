//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/math.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_sign)
{
    BOOST_CHECK_EQUAL(uhd::math::sign(2.3), +1);
    BOOST_CHECK_EQUAL(uhd::math::sign(-2.3), -1);
    BOOST_CHECK_EQUAL(uhd::math::sign(0.0), 0);
}

BOOST_AUTO_TEST_CASE(test_wrap_frequency)
{
    BOOST_CHECK_EQUAL(uhd::math::wrap_frequency(10e6, 200e6), 10e6);
    BOOST_CHECK_EQUAL(uhd::math::wrap_frequency(250e6, 200e6), 50e6);
    BOOST_CHECK_EQUAL(uhd::math::wrap_frequency(120e6, 200e6), -80e6);
    BOOST_CHECK_EQUAL(uhd::math::wrap_frequency(-250e6, 200e6), -50e6);
}

//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/math.hpp>
#include <stdint.h>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_lcm)
{
    BOOST_CHECK_EQUAL(uhd::math::lcm<int>(2, 3), 6);
}

BOOST_AUTO_TEST_CASE(test_gcd)
{
    BOOST_CHECK_EQUAL(uhd::math::gcd<int>(6, 15), 3);
}

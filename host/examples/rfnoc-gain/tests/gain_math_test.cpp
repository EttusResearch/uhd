//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <iostream>

// This is a simple test, purely for demonstration purposes.
BOOST_AUTO_TEST_CASE(gain_math_test)
{
    int x = 5;
    int g = 10;

    BOOST_CHECK_EQUAL(x * g, 50);
}

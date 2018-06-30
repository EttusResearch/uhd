//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <stdint.h>
#include <uhd/utils/math.hpp>

// NOTE: This is not the only math test case, see e.g. special tests
// for fp comparison.

BOOST_AUTO_TEST_CASE(test_log2){
    double y = uhd::math::log2(16.0);
    BOOST_CHECK_EQUAL(y, 4.0);
}


//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <../lib/include/uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd;

BOOST_AUTO_TEST_CASE(test_narrow){
    uint16_t x = 5;
    uint8_t y = narrow_cast<uint8_t>(x);
    BOOST_CHECK_EQUAL(x, y);

    BOOST_CHECK_THROW(narrow<uint8_t>(uint16_t(1<<10)), narrowing_error);
    BOOST_CHECK_THROW(narrow<uint8_t>(int8_t(-1)), narrowing_error);
}

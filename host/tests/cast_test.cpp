//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <stdint.h>
#include <uhd/utils/cast.hpp>

BOOST_AUTO_TEST_CASE(test_mac_addr){
    std::string in = "0x0100";
    uint16_t correct_result = 256;
    uint16_t x = uhd::cast::hexstr_cast<uint16_t>(in);
    //uint16_t x = uhd::cast::hexstr_cast(in);
    std::cout
        << "Testing hex -> uint16_t conversion. "
        << in << " == " << std::hex << x << "?" << std::endl;
    BOOST_CHECK_EQUAL(x, correct_result);
}


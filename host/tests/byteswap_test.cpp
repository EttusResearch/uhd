//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhd/utils/byteswap.hpp>

BOOST_AUTO_TEST_CASE(test_byteswap16){
    uint16_t x = 0x0123;
    uint16_t y = 0x2301;
    BOOST_CHECK_EQUAL(uhd::byteswap(x), y);
}

BOOST_AUTO_TEST_CASE(test_byteswap32){
    uint32_t x = 0x01234567;
    uint32_t y = 0x67452301;
    BOOST_CHECK_EQUAL(uhd::byteswap(x), y);
}

BOOST_AUTO_TEST_CASE(test_byteswap64){
    //split up 64 bit constants to avoid long-long compiler warnings
    uint64_t x = 0x01234567 | (uint64_t(0x89abcdef) << 32);
    uint64_t y = 0xefcdab89 | (uint64_t(0x67452301) << 32);
    BOOST_CHECK_EQUAL(uhd::byteswap(x), y);
}


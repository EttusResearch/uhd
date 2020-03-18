//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/cast.hpp>
#include <stdint.h>
#include <boost/test/unit_test.hpp>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_mac_addr)
{
    std::string in          = "0x0100";
    uint16_t correct_result = 256;
    uint16_t x              = uhd::cast::hexstr_cast<uint16_t>(in);
    // uint16_t x = uhd::cast::hexstr_cast(in);
    std::cout << "Testing hex -> uint16_t conversion. " << in << " == " << std::hex << x
              << "?" << std::endl;
    BOOST_CHECK_EQUAL(x, correct_result);
}

BOOST_AUTO_TEST_CASE(test_from_str)
{
    using namespace uhd::cast;
    BOOST_CHECK_EQUAL(5.0, from_str<double>("5.0"));
    BOOST_CHECK_EQUAL(23, from_str<int>("23"));
    BOOST_CHECK_EQUAL("foo", from_str<std::string>("foo"));

    BOOST_CHECK_EQUAL(true, from_str<bool>("true"));
    BOOST_CHECK_EQUAL(true, from_str<bool>("True"));
    BOOST_CHECK_EQUAL(true, from_str<bool>("Y"));
    BOOST_CHECK_EQUAL(true, from_str<bool>("y"));
    BOOST_CHECK_EQUAL(true, from_str<bool>("YES"));
    BOOST_CHECK_EQUAL(true, from_str<bool>("yEs"));
    BOOST_CHECK_EQUAL(true, from_str<bool>("1"));

    BOOST_CHECK_EQUAL(false, from_str<bool>("false"));
    BOOST_CHECK_EQUAL(false, from_str<bool>("False"));
    BOOST_CHECK_EQUAL(false, from_str<bool>("n"));
    BOOST_CHECK_EQUAL(false, from_str<bool>("N"));
    BOOST_CHECK_EQUAL(false, from_str<bool>("No"));
    BOOST_CHECK_EQUAL(false, from_str<bool>("nO"));
    BOOST_CHECK_EQUAL(false, from_str<bool>("0"));

    BOOST_CHECK_THROW(from_str<bool>(""), uhd::runtime_error);
    BOOST_CHECK_THROW(from_str<bool>("abc"), uhd::runtime_error);
    BOOST_CHECK_THROW(from_str<bool>("deadbeef"), uhd::runtime_error);
}

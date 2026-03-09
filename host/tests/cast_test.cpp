//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/device_addr.hpp>
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

BOOST_AUTO_TEST_CASE(test_to_str)
{
    using namespace uhd::cast;

    // Test integer type specializations
    BOOST_CHECK_EQUAL("42", to_str(42));
    BOOST_CHECK_EQUAL("-17", to_str(-17));
    BOOST_CHECK_EQUAL("123456", to_str(123456U));
    BOOST_CHECK_EQUAL("0", to_str(0U));
    BOOST_CHECK_EQUAL("4294967295", to_str(UINT32_MAX));
    BOOST_CHECK_EQUAL("-32768", to_str(INT16_MIN));
    BOOST_CHECK_EQUAL("255", to_str(static_cast<uint8_t>(255)));
    BOOST_CHECK_EQUAL("-128", to_str(static_cast<int8_t>(-128)));
    BOOST_CHECK_EQUAL("9223372036854775807", to_str(INT64_MAX));

    // Test device_addr_t specialization
    uhd::device_addr_t dev_addr("key1=val1,key2=val2");
    BOOST_CHECK_EQUAL("key1=val1,key2=val2", to_str(dev_addr));

    uhd::device_addr_t empty_dev_addr;
    BOOST_CHECK_EQUAL("", to_str(empty_dev_addr));

    // Test floating-point type specializations
    // Test that conversion works and round-trip is accurate rather than exact format
    std::string double_str = to_str(3.14159);
    std::string float_str  = to_str(2.5f);
    BOOST_CHECK(!double_str.empty());
    BOOST_CHECK(!float_str.empty());
    BOOST_CHECK_CLOSE(
        3.14159, from_str<double>(double_str), 1e-12); // Use BOOST_CHECK_CLOSE for safety
    BOOST_CHECK_CLOSE(
        2.5f, std::stof(float_str), 1e-5); // Use BOOST_CHECK_CLOSE for safety

    // Test round-trip safety for high precision floating-point values
    // that require more than std::to_string's default 6-digit precision
    double high_precision_value    = 1.0 / 3.0; // 0.3333333...
    std::string high_precision_str = to_str(high_precision_value);
    double round_trip_value        = from_str<double>(high_precision_str);
    BOOST_CHECK_CLOSE(high_precision_value, round_trip_value, 1e-12); // 1e-12% tolerance

    // Test with a value near max_digits10 precision
    double max_precision_value    = 1.23456789012345678;
    std::string max_precision_str = to_str(max_precision_value);
    double max_round_trip_value   = from_str<double>(max_precision_str);
    BOOST_CHECK_CLOSE(
        max_precision_value, max_round_trip_value, 1e-12); // 1e-12% tolerance

    // Test with float precision
    float float_high_precision = 1.0f / 7.0f;
    std::string float_high_str = to_str(float_high_precision);
    float float_round_trip     = std::stof(float_high_str);
    BOOST_CHECK_CLOSE(
        float_high_precision, float_round_trip, 1e-5); // 1e-5% tolerance for float

    // Test string overloads
    BOOST_CHECK_EQUAL("hello", to_str(std::string("hello")));
    BOOST_CHECK_EQUAL("world", to_str("world"));

    // Test roundtrip conversion for consistency
    BOOST_CHECK_EQUAL(42, from_str<int>(to_str<int>(42)));
    BOOST_CHECK_EQUAL(1024U, from_str<size_t>(to_str<size_t>(1024U)));
}

//
// Copyright 2020 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/serial_number.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_serial_numbers_match)
{
    BOOST_CHECK_EQUAL(true, uhd::utils::serial_numbers_match("abcd123", "abcd123"));
    BOOST_CHECK_EQUAL(true, uhd::utils::serial_numbers_match("0abcd123", "0abcd123"));
    BOOST_CHECK_EQUAL(false, uhd::utils::serial_numbers_match("0abcd123", "abcd1230"));
    BOOST_CHECK_EQUAL(false, uhd::utils::serial_numbers_match("abcd123", "abcd124"));
    BOOST_CHECK_EQUAL(false, uhd::utils::serial_numbers_match("abcd123", "321dcba"));
    BOOST_CHECK_EQUAL(true, uhd::utils::serial_numbers_match("abcd123", "0abcd123"));
    BOOST_CHECK_EQUAL(true, uhd::utils::serial_numbers_match("0abcd123", "abcd123"));

    // Out of range
    BOOST_CHECK_EQUAL(
        false, uhd::utils::serial_numbers_match("aaaaaaaaaaaaaa", "abcd123"));

    // Invalid argument
    BOOST_CHECK_EQUAL(false, uhd::utils::serial_numbers_match("", "abcd123"));
}

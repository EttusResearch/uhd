//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/compat_check.hpp>
#include <boost/test/unit_test.hpp>

using namespace uhd;

BOOST_AUTO_TEST_CASE(test_compat_num16)
{
    compat_num16 cn{5, 3};
    BOOST_CHECK(cn == compat_num16(5, 3));
    BOOST_CHECK(cn < compat_num16(5, 4));
    BOOST_CHECK(cn < compat_num16(6, 0));
    BOOST_CHECK(cn <= compat_num16(6, 0));
    BOOST_CHECK(cn <= compat_num16(5, 3));
    BOOST_CHECK(cn > compat_num16(5, 2));
    BOOST_CHECK(cn > compat_num16(4, 7));
    BOOST_CHECK(cn >= compat_num16(4, 7));
    BOOST_CHECK(cn >= compat_num16(5, 3));
    BOOST_CHECK(cn != compat_num16(0, 0));
    BOOST_CHECK(cn == compat_num16((5 << 8) | 3));
}

BOOST_AUTO_TEST_CASE(test_compat_num32)
{
    compat_num32 cn{5, 3};
    BOOST_CHECK(cn == compat_num32(5, 3));
    BOOST_CHECK(cn < compat_num32(5, 4));
    BOOST_CHECK(cn < compat_num32(6, 0));
    BOOST_CHECK(cn <= compat_num32(6, 0));
    BOOST_CHECK(cn <= compat_num32(5, 3));
    BOOST_CHECK(cn > compat_num32(5, 2));
    BOOST_CHECK(cn > compat_num32(4, 7));
    BOOST_CHECK(cn >= compat_num32(4, 7));
    BOOST_CHECK(cn >= compat_num32(5, 3));
    BOOST_CHECK(cn != compat_num32(0, 0));
    BOOST_CHECK(cn == compat_num32((5 << 16) | 3));
}


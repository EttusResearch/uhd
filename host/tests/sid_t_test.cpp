//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <sstream>
#include <boost/test/unit_test.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/exception.hpp>

using uhd::sid_t;

BOOST_AUTO_TEST_CASE(test_sid_t) {
    uint32_t sid_value = 0x01020310;
    sid_t sid(sid_value);

    BOOST_CHECK_EQUAL(sid.is_set(), true);
    BOOST_CHECK_EQUAL(sid.to_pp_string(), "1.2>3.16");
    BOOST_CHECK_EQUAL(sid.to_pp_string_hex(), "01:02>03:10");
    BOOST_CHECK_EQUAL(sid.get_src(), (uint32_t)0x0102);
    BOOST_CHECK_EQUAL(sid.get_dst(), (uint32_t)0x0310);
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)0x01);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)0x02);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)0x03);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)0x10);
    BOOST_CHECK_EQUAL(sid == sid, true);
    BOOST_CHECK_EQUAL(sid == sid_value, true);

    uint32_t check_sid_val = (uint32_t) sid;
    BOOST_CHECK_EQUAL(check_sid_val, sid_value);

    std::stringstream ss_dec;
    ss_dec << sid;
    BOOST_CHECK_EQUAL(ss_dec.str(), "1.2>3.16");

    std::stringstream ss_hex;
    ss_hex << std::hex << sid;
    BOOST_CHECK_EQUAL(ss_hex.str(), "01:02>03:10");

    sid_t empty_sid;
    BOOST_CHECK_EQUAL(empty_sid.is_set(), false);
    BOOST_CHECK_EQUAL(empty_sid.to_pp_string(), "x.x>x.x");
    BOOST_CHECK_EQUAL(empty_sid.to_pp_string_hex(), "xx:xx>xx:xx");
    BOOST_CHECK_EQUAL(empty_sid == sid, false);
    BOOST_CHECK_EQUAL(empty_sid == sid_value, false);
    BOOST_CHECK_EQUAL((bool) empty_sid, false);

    empty_sid = sid_value; // No longer empty
    BOOST_CHECK_EQUAL(empty_sid.is_set(), true);
    BOOST_CHECK_EQUAL(empty_sid == sid, true);
}

BOOST_AUTO_TEST_CASE(test_sid_t_set) {
    uint32_t sid_value = 0x0;
    sid_t sid(sid_value);

    sid.set(0x01020304);
    BOOST_CHECK_EQUAL(sid.get(), (uint32_t)0x01020304);
    BOOST_CHECK_EQUAL(sid.get_src_addr(),(uint32_t)0x01);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)0x02);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)0x03);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)0x04);
    BOOST_CHECK_EQUAL(sid.get_dst_xbarport(), (uint32_t)0x0);
    BOOST_CHECK_EQUAL(sid.get_dst_blockport(), (uint32_t)0x4);

    sid.set_src_addr(0x0a);
    BOOST_CHECK_EQUAL(sid.get(), (uint32_t)0x0a020304);
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)0x0a);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)0x02);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)0x03);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)0x04);

    sid.set_src_endpoint(0x0b);
    BOOST_CHECK_EQUAL(sid.get(), (uint32_t)0x0a0b0304);
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)0x0a);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)0x0b);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)0x03);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)0x04);

    sid.set_dst_addr(0x0c);
    BOOST_CHECK_EQUAL(sid.get(), (uint32_t)0x0a0b0c04);
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)0x0a);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)0x0b);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)0x0c);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)0x04);

    sid.set_dst_endpoint(0x0d);
    BOOST_CHECK_EQUAL(sid.get(), (uint32_t)0x0a0b0c0d);
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)0x0a);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)0x0b);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)0x0c);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)0x0d);

    sid.set_dst_xbarport(0xb);
    BOOST_CHECK_EQUAL(sid.get(), (uint32_t)0x0a0b0cbd);
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)0x0a);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)0x0b);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)0x0c);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)0xbd);

    sid.set_dst_blockport(0xc);
    BOOST_CHECK_EQUAL(sid.get(), (uint32_t)0x0a0b0cbc);
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)0x0a);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)0x0b);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)0x0c);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)0xbc);

    const sid_t flipped_sid = sid.reversed();
    BOOST_CHECK_EQUAL(flipped_sid.get(), (uint32_t)0x0cbc0a0b);
    BOOST_CHECK_EQUAL(flipped_sid.reversed(), sid);

    // In-place
    sid.reverse();
    BOOST_CHECK_EQUAL(sid.get(), (uint32_t)0x0cbc0a0b);
}

BOOST_AUTO_TEST_CASE(test_sid_t_from_str) {
    sid_t sid("1.2>3.4");
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)1);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)2);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)3);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)4);

    sid = "01:02>03:10";
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)1);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)2);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)3);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)16);

    sid = "01:06/03:10";
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)1);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)6);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)3);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)16);

    sid = "01:02:04:10";
    BOOST_CHECK_EQUAL(sid.get_src_addr(), (uint32_t)1);
    BOOST_CHECK_EQUAL(sid.get_src_endpoint(), (uint32_t)2);
    BOOST_CHECK_EQUAL(sid.get_dst_addr(), (uint32_t)4);
    BOOST_CHECK_EQUAL(sid.get_dst_endpoint(), (uint32_t)16);

    BOOST_REQUIRE_THROW(sid_t fail_sid("foobar"), uhd::value_error);
    BOOST_REQUIRE_THROW(sid_t fail_sid("01:02:03:4"), uhd::value_error);
    BOOST_REQUIRE_THROW(sid_t fail_sid("01:02:03:004"), uhd::value_error);
    BOOST_REQUIRE_THROW(sid_t fail_sid("1.2.3.0004"), uhd::value_error);
}

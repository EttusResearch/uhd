//
// Copyright 2014-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/stream_sig.hpp>

using namespace uhd::rfnoc;

BOOST_AUTO_TEST_CASE(test_stream_sig) {
    stream_sig_t stream_sig;

    BOOST_CHECK_EQUAL(stream_sig.item_type, "");
    BOOST_CHECK_EQUAL(stream_sig.vlen, 0);
    BOOST_CHECK_EQUAL(stream_sig.packet_size, 0);
    BOOST_CHECK_EQUAL(stream_sig.is_bursty, false);

    std::stringstream ss;
    ss << stream_sig;
    // Eventually actually test the contents
    std::cout << ss.str() << std::endl;
}

BOOST_AUTO_TEST_CASE(test_stream_sig_compat) {
    stream_sig_t upstream_sig;
    stream_sig_t downstream_sig;

    BOOST_CHECK(stream_sig_t::is_compatible(upstream_sig, downstream_sig));
    upstream_sig.vlen = 32;
    BOOST_CHECK(stream_sig_t::is_compatible(upstream_sig, downstream_sig));
    downstream_sig.vlen = 32;
    BOOST_CHECK(stream_sig_t::is_compatible(upstream_sig, downstream_sig));
    upstream_sig.vlen = 16;
    BOOST_CHECK(not stream_sig_t::is_compatible(upstream_sig, downstream_sig));
    upstream_sig.vlen = 32;
    BOOST_CHECK(stream_sig_t::is_compatible(upstream_sig, downstream_sig));
    upstream_sig.packet_size = 8;
    BOOST_CHECK(stream_sig_t::is_compatible(upstream_sig, downstream_sig));
    downstream_sig.packet_size = 12;
    BOOST_CHECK(not stream_sig_t::is_compatible(upstream_sig, downstream_sig));
    upstream_sig.packet_size = 0;
    BOOST_CHECK(stream_sig_t::is_compatible(upstream_sig, downstream_sig));
    downstream_sig.item_type = "";
    BOOST_CHECK(stream_sig_t::is_compatible(upstream_sig, downstream_sig));
    upstream_sig.item_type = "sc16";
    downstream_sig.item_type = "s8";
    BOOST_CHECK(not stream_sig_t::is_compatible(upstream_sig, downstream_sig));
}

BOOST_AUTO_TEST_CASE(test_stream_sig_types) {
    stream_sig_t stream_sig;
    BOOST_CHECK_EQUAL(stream_sig.get_bytes_per_item(), 0);
    stream_sig.item_type = "sc16";
    BOOST_CHECK_EQUAL(stream_sig.get_bytes_per_item(), 4);
    stream_sig.item_type = "sc12";
    BOOST_CHECK_EQUAL(stream_sig.get_bytes_per_item(), 3);
    stream_sig.item_type = "sc8";
    BOOST_CHECK_EQUAL(stream_sig.get_bytes_per_item(), 2);
    stream_sig.item_type = "s16";
    BOOST_CHECK_EQUAL(stream_sig.get_bytes_per_item(), 2);
    stream_sig.item_type = "s8";
    BOOST_CHECK_EQUAL(stream_sig.get_bytes_per_item(), 1);
    stream_sig.item_type = "fc32";
    BOOST_CHECK_EQUAL(stream_sig.get_bytes_per_item(), 8);
    stream_sig.item_type = "not_a_type";
    BOOST_REQUIRE_THROW(stream_sig.get_bytes_per_item(), uhd::key_error);
}

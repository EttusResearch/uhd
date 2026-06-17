//
// Copyright 2026 Ettus Research
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/utils/device_filter.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(test_device_addr_matches_missing_product)
{
    bool matches = false;
    BOOST_REQUIRE_NO_THROW(
        matches = uhd::device_filter::device_addr_matches(
            uhd::device_addr_t("product=x4xx"), uhd::device_addr_t("addr=192.168.10.2")));
    BOOST_CHECK(matches);
}

BOOST_AUTO_TEST_CASE(test_device_addr_matches_type_sim)
{
    BOOST_CHECK(uhd::device_filter::device_addr_matches(
        uhd::device_addr_t("type=sim"), uhd::device_addr_t("addr=192.168.10.2")));
}

BOOST_AUTO_TEST_CASE(test_device_addr_matches_missing_serial_does_not_throw)
{
    bool matches = true;
    BOOST_REQUIRE_NO_THROW(matches = uhd::device_filter::device_addr_matches(
                               uhd::device_addr_t("serial=abcd123"),
                               uhd::device_addr_t("addr=192.168.10.2")));
    BOOST_CHECK(!matches);
}

BOOST_AUTO_TEST_CASE(test_filter_device_addrs)
{
    const uhd::device_addrs_t discovered_addrs{
        uhd::device_addr_t("addr=192.168.10.2"), uhd::device_addr_t("product=n3xx")};
    const auto filtered_addrs = uhd::device_filter::filter_device_addrs(
        discovered_addrs, uhd::device_addr_t("product=x4xx"));

    BOOST_REQUIRE_EQUAL(filtered_addrs.size(), 1);
    BOOST_CHECK_EQUAL(filtered_addrs.front().to_string(), "addr=192.168.10.2");
}

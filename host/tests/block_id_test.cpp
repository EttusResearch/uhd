//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <iostream>
#include <boost/test/unit_test.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/block_id.hpp>

using namespace uhd::rfnoc;

BOOST_AUTO_TEST_CASE(test_block_id) {
    BOOST_CHECK(block_id_t::is_valid_block_id("00/Filter_1"));
    BOOST_CHECK(not block_id_t::is_valid_block_id("0/MAG_SQUARE"));
    BOOST_CHECK(block_id_t::is_valid_blockname("FilterFoo"));
    BOOST_CHECK(not block_id_t::is_valid_blockname("Filter_Foo"));
    BOOST_CHECK(not block_id_t::is_valid_blockname("Filter/Foo"));
    BOOST_CHECK(not block_id_t::is_valid_blockname("0Filter/Foo"));
    BOOST_CHECK(not block_id_t::is_valid_blockname("0/Filter/Foo"));

    BOOST_REQUIRE_THROW(block_id_t invalid_block_id("0Filter/1"), uhd::value_error);

    block_id_t block_id("0/FFT_1");
    BOOST_CHECK_EQUAL(block_id.get_device_no(), 0);
    BOOST_CHECK_EQUAL(block_id.get_block_name(), "FFT");
    BOOST_CHECK_EQUAL(block_id.get_block_count(), 1);

    block_id.set_device_no(17);
    BOOST_CHECK_EQUAL(block_id.get_device_no(), 17);
    BOOST_CHECK_EQUAL(block_id.get_block_name(), "FFT");
    BOOST_CHECK_EQUAL(block_id.get_block_count(), 1);

    block_id.set_block_count(11);
    BOOST_CHECK_EQUAL(block_id.get_device_no(), 17);
    BOOST_CHECK_EQUAL(block_id.get_block_name(), "FFT");
    BOOST_CHECK_EQUAL(block_id.get_block_count(), 11);

    block_id.set_block_name("FooBar");
    BOOST_CHECK_EQUAL(block_id.get_device_no(), 17);
    BOOST_CHECK_EQUAL(block_id.get_block_name(), "FooBar");
    BOOST_CHECK_EQUAL(block_id.get_block_count(), 11);

    BOOST_CHECK(not block_id.set_block_name("Foo_Bar"));
    BOOST_CHECK_EQUAL(block_id.get_device_no(), 17);
    BOOST_CHECK_EQUAL(block_id.get_block_name(), "FooBar"); // Is unchanged because invalid
    BOOST_CHECK_EQUAL(block_id.get_block_count(), 11);

    block_id++;
    BOOST_CHECK_EQUAL(block_id.get_device_no(), 17);
    BOOST_CHECK_EQUAL(block_id.get_block_name(), "FooBar");
    BOOST_CHECK_EQUAL(block_id.get_block_count(), 12);

    block_id_t other_block_id(7, "BlockName", 3);
    BOOST_CHECK_EQUAL(other_block_id.get_device_no(), 7);
    BOOST_CHECK_EQUAL(other_block_id.get_block_name(), "BlockName");
    BOOST_CHECK_EQUAL(other_block_id.get_block_count(), 3);
    BOOST_CHECK_EQUAL(other_block_id.to_string(), "7/BlockName_3");

    // Cast
    std::string block_id_str = std::string(other_block_id);
    std::cout << "Should print '7/BlockName_3': " << block_id_str << std::endl;
    BOOST_CHECK_EQUAL(block_id_str, "7/BlockName_3");

    // Operators
    std::cout << "Testing ostream printing (<<): " << other_block_id << std::endl;
    BOOST_CHECK_EQUAL(other_block_id, block_id_str);
    BOOST_CHECK_EQUAL(other_block_id, "7/BlockName_3");

    // match()
    BOOST_CHECK(other_block_id.match("BlockName"));
    BOOST_CHECK(other_block_id.match("7/BlockName"));
    BOOST_CHECK(other_block_id.match("BlockName_3"));
    BOOST_CHECK(other_block_id.match("7/BlockName_3"));
    BOOST_CHECK(not other_block_id.match("8/BlockName"));
    BOOST_CHECK(not other_block_id.match("8/BlockName_3"));
    BOOST_CHECK(not other_block_id.match("Block_Name_3"));
    BOOST_CHECK(not other_block_id.match("BlockName_4"));
    BOOST_CHECK(not other_block_id.match("BlockName_X"));
    BOOST_CHECK(not other_block_id.match("2093ksdjfflsdkjf"));
}

BOOST_AUTO_TEST_CASE(test_block_id_set) {
    // test set()
    block_id_t block_id_for_set(5, "Blockname", 9);
    block_id_for_set.set("FirFilter");
    BOOST_CHECK_EQUAL(block_id_for_set.get_device_no(), 5);
    BOOST_CHECK_EQUAL(block_id_for_set.get_block_name(), "FirFilter");
    BOOST_CHECK_EQUAL(block_id_for_set.get_block_count(), 9);
    block_id_for_set.set("1/FirFilter2");
    BOOST_CHECK_EQUAL(block_id_for_set.get_device_no(), 1);
    BOOST_CHECK_EQUAL(block_id_for_set.get_block_name(), "FirFilter2");
    BOOST_CHECK_EQUAL(block_id_for_set.get_block_count(), 9);
    block_id_for_set.set("Sync_3");
    BOOST_CHECK_EQUAL(block_id_for_set.get_device_no(), 1);
    BOOST_CHECK_EQUAL(block_id_for_set.get_block_name(), "Sync");
    BOOST_CHECK_EQUAL(block_id_for_set.get_block_count(), 3);
}

BOOST_AUTO_TEST_CASE(test_block_id_cmp) {
    BOOST_CHECK(block_id_t("0/FFT_1") == block_id_t("0/FFT_1"));
    BOOST_CHECK(block_id_t("0/FFT_1") != block_id_t("1/FFT_1"));
    BOOST_CHECK(block_id_t("0/FFT_1") < block_id_t("1/aaaaaaaaa_0"));
    BOOST_CHECK(not (block_id_t("0/FFT_1") > block_id_t("1/aaaaaaaaa_0")));
}

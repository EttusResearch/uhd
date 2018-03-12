//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/chdr.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/format.hpp>
#include <cstdlib>
#include <iostream>

using namespace uhd::transport::vrt;

static void pack_and_unpack(
    if_packet_info_t &if_packet_info_in
){
    // Temp buffer for packed packet
    uint32_t packet_buff[2048] = {0};

    // Check input (must not be lazy)
    BOOST_REQUIRE(
        (if_packet_info_in.num_payload_words32 == 0 and if_packet_info_in.num_payload_bytes == 0)
        or
        (if_packet_info_in.num_payload_words32 != 0 and if_packet_info_in.num_payload_bytes != 0)
    );
    if (if_packet_info_in.num_payload_words32) {
        BOOST_REQUIRE(if_packet_info_in.num_payload_bytes <= 4 * if_packet_info_in.num_payload_words32);
        BOOST_REQUIRE(if_packet_info_in.num_payload_bytes > 4*(if_packet_info_in.num_payload_words32-1));
    }

    //pack metadata into a vrt header
    chdr::if_hdr_pack_be(
        packet_buff, if_packet_info_in
    );
    std::cout << std::endl;
    uint32_t header_bits = (uhd::ntohx(packet_buff[0]) >> 28);
    std::cout << boost::format("header bits = 0b%d%d%d%d") % ((header_bits & 8) > 0)
                                                           % ((header_bits & 4) > 0)
                                                           % ((header_bits & 2) > 0)
                                                           % ((header_bits & 1) > 0) << std::endl;
    for (size_t i = 0; i < 5; i++)
    {
        std::cout << boost::format("packet_buff[%u] = 0x%08x") % i % uhd::ntohx(packet_buff[i]) << std::endl;
    }

    if_packet_info_t if_packet_info_out;
    // Must be set a-priori as per contract
    if_packet_info_out.num_packet_words32 = if_packet_info_in.num_packet_words32;

    //unpack the vrt header back into metadata
    chdr::if_hdr_unpack_be(
        packet_buff, if_packet_info_out
    );

    //check the the unpacked metadata is the same
    BOOST_CHECK_EQUAL(if_packet_info_in.packet_count, if_packet_info_out.packet_count);
    BOOST_CHECK_EQUAL(if_packet_info_in.num_header_words32, if_packet_info_out.num_header_words32);
    BOOST_CHECK_EQUAL(if_packet_info_in.num_payload_words32, if_packet_info_out.num_payload_words32);
    BOOST_CHECK(if_packet_info_out.has_sid);
    BOOST_CHECK_EQUAL(if_packet_info_in.sid, if_packet_info_out.sid);
    BOOST_CHECK(if_packet_info_out.has_sid);
    BOOST_CHECK_EQUAL(if_packet_info_in.has_tsf, if_packet_info_out.has_tsf);
    if (if_packet_info_in.has_tsf and if_packet_info_out.has_tsf){
        BOOST_CHECK_EQUAL(if_packet_info_in.tsf, if_packet_info_out.tsf);
    }
}

BOOST_AUTO_TEST_CASE(test_with_chdr){
    if_packet_info_t if_packet_info;
    if_packet_info.packet_type = if_packet_info_t::PACKET_TYPE_DATA;
    if_packet_info.eob = false;
    if_packet_info.packet_count = 7;
    if_packet_info.has_tsf = true;
    if_packet_info.tsf = 0x1234567890ABCDEFull;
    if_packet_info.sid = 0xAABBCCDD;
    if_packet_info.num_payload_words32 = 24;
    if_packet_info.num_payload_bytes = 95;
    pack_and_unpack(if_packet_info);
}

BOOST_AUTO_TEST_CASE(test_with_chdr_fc){
    if_packet_info_t if_packet_info;
    if_packet_info.packet_type = if_packet_info_t::PACKET_TYPE_FC;
    if_packet_info.eob = false;
    if_packet_info.packet_count = 19;
    if_packet_info.has_tsf = false;
    if_packet_info.tsf = 0x1234567890ABCDEFull;
    if_packet_info.sid = 0xAABBCCDD;
    if_packet_info.num_payload_words32 = 4;
    if_packet_info.num_payload_bytes = 16;
    pack_and_unpack(if_packet_info);
}

BOOST_AUTO_TEST_CASE(test_with_chdr_cmd){
    if_packet_info_t if_packet_info;
    if_packet_info.packet_type = if_packet_info_t::PACKET_TYPE_CMD;
    if_packet_info.packet_count = 19;
    if_packet_info.has_tsf = true;
    if_packet_info.tsf = 0x1234567890ABCDEFull;
    if_packet_info.sid = 0xAABBCCDD;
    if_packet_info.num_payload_words32 = 4;
    if_packet_info.num_payload_bytes = 16;
    pack_and_unpack(if_packet_info);
}

BOOST_AUTO_TEST_CASE(test_with_chdr_resp){
    if_packet_info_t if_packet_info;
    if_packet_info.packet_type = if_packet_info_t::PACKET_TYPE_RESP;
    if_packet_info.packet_count = 123;
    if_packet_info.has_tsf = false;
    if_packet_info.tsf = 0x1234567890ABCDEFull;
    if_packet_info.sid = 0xAABBCCDD;
    if_packet_info.num_payload_words32 = 4;
    if_packet_info.num_payload_bytes = 16;
    pack_and_unpack(if_packet_info);
}

BOOST_AUTO_TEST_CASE(test_with_chdr_err){
    if_packet_info_t if_packet_info;
    if_packet_info.packet_type = if_packet_info_t::PACKET_TYPE_ERROR;
    if_packet_info.packet_count = 1928;
    if_packet_info.eob = false;
    if_packet_info.error = false; // Needs to be set explicitly
    if_packet_info.has_tsf = false;
    if_packet_info.tsf = 0x1234567890ABCDEFull;
    if_packet_info.sid = 0xAABBCCDD;
    if_packet_info.num_payload_words32 = 4;
    if_packet_info.num_payload_bytes = 16;
    pack_and_unpack(if_packet_info);
}


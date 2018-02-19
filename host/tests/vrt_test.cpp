//
// Copyright 2010-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/format.hpp>
#include <cstdlib>
#include <iostream>

using namespace uhd::transport;

static void pack_and_unpack(
    vrt::if_packet_info_t &if_packet_info_in
){
    if (if_packet_info_in.num_payload_bytes == 0)
    {
        if_packet_info_in.num_payload_bytes = if_packet_info_in.num_payload_words32 * sizeof(uint32_t);
    }
    uint32_t packet_buff[2048];

    //pack metadata into a vrt header
    vrt::if_hdr_pack_be(
        packet_buff, if_packet_info_in
    );
    std::cout << std::endl;
    for (size_t i = 0; i < 5; i++)
    {
        std::cout << boost::format("packet_buff[%u] = 0x%.8x") % i % uhd::byteswap(packet_buff[i]) << std::endl;
    }

    vrt::if_packet_info_t if_packet_info_out;
    if_packet_info_out.link_type = if_packet_info_in.link_type;
    if_packet_info_out.num_packet_words32 = if_packet_info_in.num_packet_words32;

    //unpack the vrt header back into metadata
    vrt::if_hdr_unpack_be(
        packet_buff, if_packet_info_out
    );

    //check the the unpacked metadata is the same
    BOOST_CHECK_EQUAL(if_packet_info_in.packet_count, if_packet_info_out.packet_count);
    BOOST_CHECK_EQUAL(if_packet_info_in.num_header_words32, if_packet_info_out.num_header_words32);
    BOOST_CHECK_EQUAL(if_packet_info_in.num_payload_words32, if_packet_info_out.num_payload_words32);
    BOOST_CHECK_EQUAL(if_packet_info_in.has_sid, if_packet_info_out.has_sid);
    if (if_packet_info_in.has_sid and if_packet_info_out.has_sid){
        BOOST_CHECK_EQUAL(if_packet_info_in.sid, if_packet_info_out.sid);
    }
    BOOST_CHECK_EQUAL(if_packet_info_in.has_cid, if_packet_info_out.has_cid);
    if (if_packet_info_in.has_cid and if_packet_info_out.has_cid){
        BOOST_CHECK_EQUAL(if_packet_info_in.cid, if_packet_info_out.cid);
    }
    BOOST_CHECK_EQUAL(if_packet_info_in.has_tsi, if_packet_info_out.has_tsi);
    if (if_packet_info_in.has_tsi and if_packet_info_out.has_tsi){
        BOOST_CHECK_EQUAL(if_packet_info_in.tsi, if_packet_info_out.tsi);
    }
    BOOST_CHECK_EQUAL(if_packet_info_in.has_tsf, if_packet_info_out.has_tsf);
    if (if_packet_info_in.has_tsf and if_packet_info_out.has_tsf){
        BOOST_CHECK_EQUAL(if_packet_info_in.tsf, if_packet_info_out.tsf);
    }
    BOOST_CHECK_EQUAL(if_packet_info_in.has_tlr, if_packet_info_out.has_tlr);
    if (if_packet_info_in.has_tlr and if_packet_info_out.has_tlr){
        BOOST_CHECK_EQUAL(if_packet_info_in.tlr, if_packet_info_out.tlr);
    }
}

/***********************************************************************
 * Loopback test the vrt packer/unpacker with various packet info combos
 * The trailer is not tested as it is not convenient to do so.
 **********************************************************************/

BOOST_AUTO_TEST_CASE(test_with_none){
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.packet_count = 0;
    if_packet_info.has_sid = false;
    if_packet_info.has_cid = false;
    if_packet_info.has_tsi = false;
    if_packet_info.has_tsf = false;
    if_packet_info.has_tlr = false;
    if_packet_info.num_payload_words32 = 0;
    pack_and_unpack(if_packet_info);
}

BOOST_AUTO_TEST_CASE(test_with_sid){
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.packet_count = 1;
    if_packet_info.has_sid = true;
    if_packet_info.has_cid = false;
    if_packet_info.has_tsi = false;
    if_packet_info.has_tsf = false;
    if_packet_info.has_tlr = false;
    if_packet_info.sid = std::rand();
    if_packet_info.num_payload_words32 = 11;
    pack_and_unpack(if_packet_info);
}

static const bool cid_enb = false;

BOOST_AUTO_TEST_CASE(test_with_cid){
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.packet_count = 2;
    if_packet_info.has_sid = false;
    if_packet_info.has_cid = cid_enb;
    if_packet_info.has_tsi = false;
    if_packet_info.has_tsf = false;
    if_packet_info.has_tlr = false;
    if_packet_info.cid = std::rand();
    if_packet_info.num_payload_words32 = 22;
    pack_and_unpack(if_packet_info);
}

BOOST_AUTO_TEST_CASE(test_with_time){
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.packet_count = 3;
    if_packet_info.has_sid = false;
    if_packet_info.has_cid = false;
    if_packet_info.has_tsi = true;
    if_packet_info.has_tsf = true;
    if_packet_info.has_tlr = false;
    if_packet_info.tsi = std::rand();
    if_packet_info.tsf = std::rand();
    if_packet_info.num_payload_words32 = 33;
    pack_and_unpack(if_packet_info);
}

BOOST_AUTO_TEST_CASE(test_with_all){
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.packet_count = 4;
    if_packet_info.has_sid = true;
    if_packet_info.has_cid = cid_enb;
    if_packet_info.has_tsi = true;
    if_packet_info.has_tsf = true;
    if_packet_info.has_tlr = false;
    if_packet_info.sid = std::rand();
    if_packet_info.cid = std::rand();
    if_packet_info.tsi = std::rand();
    if_packet_info.tsf = std::rand();
    if_packet_info.num_payload_words32 = 44;
    pack_and_unpack(if_packet_info);
}

BOOST_AUTO_TEST_CASE(test_with_vrlp){
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_VRLP;
    if_packet_info.packet_count = 3;
    if_packet_info.has_sid = true;
    if_packet_info.has_cid = false;
    if_packet_info.has_tsi = false;
    if_packet_info.has_tsf = true;
    if_packet_info.has_tlr = true;
    if_packet_info.tsi = std::rand();
    if_packet_info.tsf = std::rand();
    if_packet_info.num_payload_words32 = 42;
    pack_and_unpack(if_packet_info);
}

BOOST_AUTO_TEST_CASE(test_with_chdr){
    vrt::if_packet_info_t if_packet_info;
    if_packet_info.link_type = vrt::if_packet_info_t::LINK_TYPE_CHDR;
    if_packet_info.packet_count = 7;
    if_packet_info.has_sid = true;
    if_packet_info.has_cid = false;
    if_packet_info.has_tsi = false;
    if_packet_info.has_tsf = true;
    if_packet_info.has_tlr = false; //tlr not suported in CHDR
    if_packet_info.tsi = std::rand();
    if_packet_info.tsf = std::rand();
    if_packet_info.num_payload_words32 = 24;
    pack_and_unpack(if_packet_info);
}

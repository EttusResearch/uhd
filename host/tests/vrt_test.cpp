//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <boost/test/unit_test.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <cstdlib>

using namespace uhd::transport;

static void pack_and_unpack(
    vrt::if_packet_info_t &if_packet_info_in
){
    boost::uint32_t header_buff[vrt::max_if_hdr_words32];

    //pack metadata into a vrt header
    vrt::if_hdr_pack_be(
        header_buff, if_packet_info_in
    );

    vrt::if_packet_info_t if_packet_info_out;
    if_packet_info_out.num_packet_words32 = if_packet_info_in.num_packet_words32;

    //unpack the vrt header back into metadata
    vrt::if_hdr_unpack_be(
        header_buff, if_packet_info_out
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
    if_packet_info.num_payload_words32 = 1111;
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
    if_packet_info.num_payload_words32 = 2222;
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
    if_packet_info.num_payload_words32 = 33333;
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
    if_packet_info.num_payload_words32 = 44444;
    pack_and_unpack(if_packet_info);
}

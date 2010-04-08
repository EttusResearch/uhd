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
#include <uhd/transport/vrt.hpp>

using namespace uhd::transport;

static void pack_and_unpack(
    const uhd::tx_metadata_t &metadata,
    size_t num_payload_words32,
    size_t packet_count
){
    boost::uint32_t header_buff[vrt::max_header_words32];
    size_t num_header_words32;
    size_t num_packet_words32;

    //pack metadata into a vrt header
    vrt::pack(
        metadata,            //input
        header_buff,         //output
        num_header_words32,  //output
        num_payload_words32, //input
        num_packet_words32,  //output
        packet_count,        //input
        100e6
    );

    uhd::rx_metadata_t metadata_out;
    size_t num_header_words32_out;
    size_t num_payload_words32_out;
    size_t packet_count_out;

    //unpack the vrt header back into metadata
    vrt::unpack(
        metadata_out,            //output
        header_buff,             //input
        num_header_words32_out,  //output
        num_payload_words32_out, //output
        num_packet_words32,      //input
        packet_count_out,        //output
        100e6
    );

    //check the the unpacked metadata is the same
    BOOST_CHECK_EQUAL(packet_count, packet_count_out);
    BOOST_CHECK_EQUAL(num_header_words32, num_header_words32_out);
    BOOST_CHECK_EQUAL(num_payload_words32, num_payload_words32_out);
    BOOST_CHECK_EQUAL(metadata.has_stream_id, metadata_out.has_stream_id);
    if (metadata.has_stream_id and metadata_out.has_stream_id){
        BOOST_CHECK_EQUAL(metadata.stream_id, metadata_out.stream_id);
    }
    BOOST_CHECK_EQUAL(metadata.has_time_spec, metadata_out.has_time_spec);
    if (metadata.has_time_spec and metadata_out.has_time_spec){
        BOOST_CHECK_EQUAL(metadata.time_spec.secs, metadata_out.time_spec.secs);
        BOOST_CHECK_EQUAL(metadata.time_spec.nsecs, metadata_out.time_spec.nsecs);
    }
}

BOOST_AUTO_TEST_CASE(test_with_none){
    uhd::tx_metadata_t metadata;
    pack_and_unpack(metadata, 300, 1);
}

BOOST_AUTO_TEST_CASE(test_with_sid){
    uhd::tx_metadata_t metadata;
    metadata.has_stream_id = true;
    metadata.stream_id = 6;
    pack_and_unpack(metadata, 400, 2);
}

BOOST_AUTO_TEST_CASE(test_with_time_spec){
    uhd::tx_metadata_t metadata;
    metadata.has_time_spec = true;
    metadata.time_spec.secs = 7;
    metadata.time_spec.nsecs = 2000;
    pack_and_unpack(metadata, 500, 3);
}

BOOST_AUTO_TEST_CASE(test_with_sid_and_time_spec){
    uhd::tx_metadata_t metadata;
    metadata.has_stream_id = true;
    metadata.stream_id = 2;
    metadata.has_time_spec = true;
    metadata.time_spec.secs = 5;
    metadata.time_spec.nsecs = 1000;
    pack_and_unpack(metadata, 600, 4);
}

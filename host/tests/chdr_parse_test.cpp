//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/utils/chdr/chdr_packet.hpp>
#include <chdr_resource/hardcoded_packets.cpp>
#include <boost/test/unit_test.hpp>
#include <bitset>
#include <iostream>
#include <memory>
#include <tuple>

constexpr uhd::rfnoc::chdr_w_t CHDR_W = uhd::rfnoc::CHDR_W_64;

namespace chdr_util  = uhd::utils::chdr;
namespace chdr_rfnoc = uhd::rfnoc::chdr;

using namespace boost::unit_test;

template <size_t bits, typename val_t>
void print_bits(val_t value)
{
    std::bitset<bits> set(value);
    uint32_t count = 0;
    for (int64_t i = set.size() - 1; i >= 0; i--) {
        bool bit = set[i];
        std::cout << (bit ? "1" : "0");
        if (count == 7) {
            count = 0;
            if (i != 0) {
                std::cout << "_";
            }
        } else {
            count++;
        }
    }
}

void assert_packet_equal(const chdr_util::chdr_packet& lhs,
    const chdr_util::chdr_packet& rhs,
    uint64_t packet_num)
{
    // Header Comparison
    auto left_header  = lhs.get_header();
    auto right_header = rhs.get_header();
    BOOST_CHECK(left_header.pack() == right_header.pack());
    if (left_header.pack() != right_header.pack()) {
        std::cout << "Packet Num " << packet_num << " - Header" << std::endl;
        std::cout << "L: ";
        print_bits<64, uint64_t>(left_header.pack());
        std::cout << " | " << left_header.to_string();
        std::cout << "R: ";
        print_bits<64, uint64_t>(right_header.pack());
        std::cout << " | " << right_header.to_string();
    }

    // Timestamp Comparison
    BOOST_CHECK(lhs.get_timestamp() == rhs.get_timestamp());

    // Payload Comparison
    BOOST_CHECK(lhs.get_payload_bytes() == rhs.get_payload_bytes());
    auto left_payload  = lhs.get_payload_bytes();
    auto right_payload = rhs.get_payload_bytes();
    if (left_payload != right_payload) {
        std::cout << "Packet Num " << packet_num << " - Payload" << std::endl;
        for (size_t i = 0; i < std::min(left_payload.size(), right_payload.size()); i++) {
            uint64_t left  = left_payload[i];
            uint64_t right = right_payload[i];
            if (right == left) {
                continue;
            }
            std::cout << "L " << i << ": ";
            print_bits<64, uint64_t>(left);
            std::cout << std::endl;
            std::cout << "R " << i << ": ";
            print_bits<64, uint64_t>(right);
            std::cout << std::endl;
        }
    }

    // Metadata Comparison
    BOOST_CHECK(lhs.get_metadata() == rhs.get_metadata());
}

std::tuple<uint8_t*, size_t>* conversations[] = {
    data_packet::peer0, data_packet::peer1, ctrl_packet::peer0, ctrl_packet::peer1};

size_t lengths[] = {data_packet::peer0_len,
    data_packet::peer1_len,
    ctrl_packet::peer0_len,
    ctrl_packet::peer1_len};

void parse_all_packets(
    std::tuple<uint8_t*, size_t>* packets, size_t packets_length, size_t conversation_num)
{
    uint8_t* bytes;
    size_t length;
    for (size_t i = 0; i < packets_length; i++) {
        BOOST_TEST_CHECKPOINT("Conversation #" << conversation_num << ", Packet #" << i);
        std::tie(bytes, length) = packets[i];
        chdr_util::chdr_packet generated_packet =
            chdr_util::chdr_packet::deserialize(CHDR_W, bytes, bytes + length);

        uint8_t packet_type = uint8_t(generated_packet.get_header().get_pkt_type());
        size_t packet_size  = generated_packet.get_header().get_length();
        BOOST_CHECK(packet_type < 8);
        BOOST_CHECK(packet_size > 0);
    }
}

BOOST_AUTO_TEST_CASE(parse_all_packets_test)
{
    for (size_t i = 0; i < 4; i++) {
        parse_all_packets(conversations[i], lengths[i], i);
    }
}

std::tuple<uint8_t*, size_t>* packet_data[] = {&ctrl_packet::peer0[19],
    &ctrl_packet::peer1[17],
    &ctrl_packet::peer0[2],
    &ctrl_packet::peer0[3],
    &ctrl_packet::peer0[6],
    &data_packet::peer0[5],
    &data_packet::peer0[8],
    &data_packet::peer1[5],
    &data_packet::peer1[29],
    &data_packet::peer1[9],
    &data_packet::eob_packet_data};

void serialize_deserialize_eq(chdr_util::chdr_packet& packet,
    std::tuple<uint8_t*, size_t>* bytes,
    size_t packet_num)
{
    uint8_t* first;
    uint8_t* last;
    size_t len;
    std::tie(first, len) = *bytes;
    last                 = first + len;

    chdr_util::chdr_packet generated_packet =
        chdr_util::chdr_packet::deserialize(CHDR_W, first, last);
    assert_packet_equal(generated_packet, packet, packet_num);

    std::vector<uint8_t> generated_bytes =
        generated_packet.serialize_to_byte_vector(uhd::ENDIANNESS_LITTLE);
    BOOST_CHECK(generated_bytes.size() == len);
    if (generated_bytes.size() == len) {
        for (size_t i = 0; i < len; i++) {
            BOOST_CHECK_MESSAGE(generated_bytes[i] == first[i], "index" << i);
        }
    }
}

BOOST_AUTO_TEST_CASE(serialize_deserialize_eq_test)
{
    for (size_t i = 0; i < 11; i++) {
        BOOST_TEST_CHECKPOINT("Packet #" << i);
        serialize_deserialize_eq(packets[i], packet_data[i], i);
    }
}

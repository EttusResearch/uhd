//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/chdr/chdr_packet.hpp>
#include <uhd/types/endianness.hpp>
#include <uhd/utils/byteswap.hpp>
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>
#include <cstring>
#include <cstdint>

using namespace uhd::rfnoc::chdr;

namespace {
    constexpr size_t MAX_BUF_SIZE = 8192;

    // Poorman's send
    void send(const void* from, void* to, size_t len)
    {
        std::memcpy(to, from, len);
    }
}

uint8_t send_packet_buff[MAX_BUF_SIZE];
uint8_t recv_packet_buff[MAX_BUF_SIZE];


template <size_t chdr_w, uhd::endianness_t endianness>
void test_loopback(packet_type_t pkt_type,
    uint16_t dst_epid,
    uint8_t flags,
    uint16_t num_md,
    size_t num_payload)
{
    // Clear buffers
    std::memset(send_packet_buff, 0, MAX_BUF_SIZE);
    std::memset(recv_packet_buff, 0, MAX_BUF_SIZE);

    auto send_header = chdr_header<chdr_w>(pkt_type);
    send_header.set_dst_epid(dst_epid);
    send_header.set_flags(flags);
    send_header.set_packet_size(num_payload, num_md);
    chdr_packet<chdr_w, endianness> send_chdr_packet(send_header, send_packet_buff);
    //chdr_packet<64, uhd::endianness_t::ENDIANNESS_LITTLE> send_chdr_packet(send_header, send_packet_buff);
    uint8_t* md_buff = send_chdr_packet.template metadata_ptr_of_type<uint8_t>();
    //* start filling in the meta data
    for (size_t i = 0; i < send_chdr_packet.get_header().get_num_bytes_metadata(); i++) {
        md_buff[i] = i + 1;
    }
    auto* pay_load = send_chdr_packet.template payload_ptr_of_type<uint8_t>();
    //* start filling in the pay load
    for (size_t i = 0; i < send_chdr_packet.get_header().get_num_bytes_payload(); i++) {
        pay_load[i] = 2 * (i + 1);
    }
    send(send_packet_buff, recv_packet_buff, MAX_BUF_SIZE);

    const chdr_packet<chdr_w, endianness> recv_chdr_packet(recv_packet_buff);
    BOOST_CHECK_EQUAL((num_md + num_payload + 1
                          + (pkt_type == packet_type_t::PACKET_TYPE_DATA_WITH_TS ? 1 : 0))
                          * (chdr_w / 8),
        recv_chdr_packet.get_header().get_length());
    BOOST_CHECK_EQUAL(flags, recv_chdr_packet.get_header().get_flags());
    BOOST_CHECK_EQUAL(dst_epid, recv_chdr_packet.get_header().get_dst_epid());
    BOOST_CHECK_EQUAL(num_md, recv_chdr_packet.get_header().get_num_metadata());
    BOOST_CHECK_EQUAL(num_payload, recv_chdr_packet.get_header().get_num_words_payload());
    BOOST_CHECK(pkt_type == recv_chdr_packet.get_header().get_pkt_type());
    const auto* out_md_buff = recv_chdr_packet.template metadata_ptr_of_type<uint8_t>();
    for (size_t i = 0; i < recv_chdr_packet.get_header().get_num_bytes_metadata(); i++) {
        BOOST_CHECK_EQUAL(md_buff[i], out_md_buff[i]);
    }
    const auto* out_payload = recv_chdr_packet.template payload_ptr_of_type<uint8_t>();
    for (size_t i = 0; i < recv_chdr_packet.get_header().get_num_bytes_payload(); i++) {
        BOOST_CHECK_EQUAL(pay_load[i], out_payload[i]);
    }
}

BOOST_AUTO_TEST_CASE(simple_read_if_chdr_pkt)
{
    constexpr size_t num_payload           = 5; // in words
    constexpr packet_type_t pkt_type       = packet_type_t::PACKET_TYPE_DATA_NO_TS;
    constexpr uint16_t num_md              = 0x5A; // 90
    constexpr uint16_t dst_epid            = 0xCAFE;
    constexpr uint8_t flags                = 0x3A;

    test_loopback<64, uhd::endianness_t::ENDIANNESS_LITTLE>(pkt_type, dst_epid, flags, num_md, num_payload);
    test_loopback<128, uhd::endianness_t::ENDIANNESS_LITTLE>(pkt_type, dst_epid, flags, num_md, num_payload);
    test_loopback<256, uhd::endianness_t::ENDIANNESS_LITTLE>(pkt_type, dst_epid, flags, num_md, num_payload);

    test_loopback<64, uhd::endianness_t::ENDIANNESS_BIG>(pkt_type, dst_epid, flags, num_md, num_payload);
    test_loopback<128, uhd::endianness_t::ENDIANNESS_BIG>(pkt_type, dst_epid, flags, num_md, num_payload);
    test_loopback<256, uhd::endianness_t::ENDIANNESS_BIG>(pkt_type, dst_epid, flags, num_md, num_payload);
}

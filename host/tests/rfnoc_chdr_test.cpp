//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/types/endianness.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::chdr;

constexpr size_t MAX_BUF_SIZE_BYTES = 1024;
constexpr size_t MAX_BUF_SIZE_WORDS = MAX_BUF_SIZE_BYTES / sizeof(uint64_t);
constexpr size_t NUM_ITERS          = 5000;

static const chdr_packet_factory chdr64_be_factory(CHDR_W_64, ENDIANNESS_BIG);
static const chdr_packet_factory chdr256_be_factory(CHDR_W_256, ENDIANNESS_BIG);
static const chdr_packet_factory chdr64_le_factory(CHDR_W_64, ENDIANNESS_LITTLE);
static const chdr_packet_factory chdr256_le_factory(CHDR_W_256, ENDIANNESS_LITTLE);

uint64_t rand64()
{
    return ((uint64_t)rand() << 32) | rand();
}

ctrl_payload populate_ctrl_payload()
{
    ctrl_payload pyld;
    pyld.dst_port    = rand64() & 0x03FF;
    pyld.src_port    = rand64() & 0x03FF;
    pyld.is_ack      = rand64() & 0x1;
    pyld.src_epid    = rand64() & 0xFFFF;
    pyld.data_vtr[0] = rand64() & 0xFFFFFFFF;
    pyld.byte_enable = rand64() & 0xF;
    pyld.op_code     = static_cast<ctrl_opcode_t>(rand64() % 8);
    pyld.status      = static_cast<ctrl_status_t>(rand64() % 4);
    if (rand64() % 2 == 0) {
        pyld.timestamp = rand64();
    } else {
        pyld.timestamp = boost::none;
    }
    return pyld;
}

strs_payload populate_strs_payload()
{
    strs_payload pyld;
    pyld.src_epid         = rand64() & 0xFFFF;
    pyld.status           = static_cast<strs_status_t>(rand64() % 4);
    pyld.capacity_bytes   = rand64() & 0xFFFFFFFFFF;
    pyld.capacity_pkts    = 0xFFFFFF;
    pyld.xfer_count_bytes = rand64();
    pyld.xfer_count_pkts  = rand64() & 0xFFFFFFFFFF;
    pyld.buff_info        = rand64() & 0xFFFF;
    pyld.status_info      = rand64() & 0xFFFFFFFFFFFF;
    return pyld;
}

strc_payload populate_strc_payload()
{
    strc_payload pyld;
    pyld.src_epid  = rand64() & 0xFFFF;
    pyld.op_code   = static_cast<strc_op_code_t>(rand64() % 3);
    pyld.op_data   = rand64() & 0xF;
    pyld.num_pkts  = rand64() & 0xFFFFFFFFFF;
    pyld.num_bytes = rand64();
    return pyld;
}

mgmt_payload populate_mgmt_payload(const chdr_w_t chdr_w)
{
    mgmt_payload pyld;
    pyld.set_header(sep_id_t(rand64() & 0xFFFF), uint16_t(rand64() & 0xFFFF), chdr_w);
    mgmt_hop_t hop;
    // management op payloads are 48 bits
    hop.add_op(mgmt_op_t(mgmt_op_t::MGMT_OP_NOP, rand64() & 0xFFFFFFFFFFFF));
    pyld.add_hop(hop);
    return pyld;
}

void byte_swap(uint64_t* buff)
{
    for (size_t i = 0; i < MAX_BUF_SIZE_WORDS; i++) {
        *(buff + i) = uhd::byteswap(*(buff + i));
    }
}

BOOST_AUTO_TEST_CASE(chdr_ctrl_packet_no_swap_64)
{
    uint64_t buff[MAX_BUF_SIZE_WORDS];

    chdr_ctrl_packet::uptr tx_pkt  = chdr64_be_factory.make_ctrl();
    chdr_ctrl_packet::cuptr rx_pkt = chdr64_be_factory.make_ctrl();

    for (size_t i = 0; i < NUM_ITERS; i++) {
        chdr_header hdr   = chdr_header(rand64());
        ctrl_payload pyld = populate_ctrl_payload();

        memset(buff, 0, MAX_BUF_SIZE_BYTES);
        tx_pkt->refresh(buff, hdr, pyld);
        BOOST_CHECK(tx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(tx_pkt->get_payload() == pyld);

        rx_pkt->refresh(buff);
        BOOST_CHECK(rx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(rx_pkt->get_payload() == pyld);

        std::cout << pyld.to_string();
    }
}

BOOST_AUTO_TEST_CASE(chdr_ctrl_packet_no_swap_256)
{
    uint64_t buff[MAX_BUF_SIZE_WORDS];

    chdr_ctrl_packet::uptr tx_pkt  = chdr256_be_factory.make_ctrl();
    chdr_ctrl_packet::cuptr rx_pkt = chdr256_be_factory.make_ctrl();

    for (size_t i = 0; i < NUM_ITERS; i++) {
        chdr_header hdr   = chdr_header(rand64());
        ctrl_payload pyld = populate_ctrl_payload();

        memset(buff, 0, MAX_BUF_SIZE_BYTES);
        tx_pkt->refresh(buff, hdr, pyld);
        BOOST_CHECK(tx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(tx_pkt->get_payload() == pyld);

        rx_pkt->refresh(buff);
        BOOST_CHECK(rx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(rx_pkt->get_payload() == pyld);
    }
}

BOOST_AUTO_TEST_CASE(chdr_ctrl_packet_swap_64)
{
    uint64_t buff[MAX_BUF_SIZE_WORDS];

    chdr_ctrl_packet::uptr tx_pkt  = chdr64_be_factory.make_ctrl();
    chdr_ctrl_packet::cuptr rx_pkt = chdr64_le_factory.make_ctrl();

    for (size_t i = 0; i < NUM_ITERS; i++) {
        chdr_header hdr   = chdr_header(rand64());
        ctrl_payload pyld = populate_ctrl_payload();

        memset(buff, 0, MAX_BUF_SIZE_BYTES);
        tx_pkt->refresh(buff, hdr, pyld);
        BOOST_CHECK(tx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(tx_pkt->get_payload() == pyld);

        byte_swap(buff);

        rx_pkt->refresh(buff);
        BOOST_CHECK(rx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(rx_pkt->get_payload() == pyld);
    }
}

BOOST_AUTO_TEST_CASE(chdr_ctrl_packet_swap_256)
{
    uint64_t buff[MAX_BUF_SIZE_WORDS];

    chdr_ctrl_packet::uptr tx_pkt  = chdr256_be_factory.make_ctrl();
    chdr_ctrl_packet::cuptr rx_pkt = chdr256_le_factory.make_ctrl();

    for (size_t i = 0; i < NUM_ITERS; i++) {
        chdr_header hdr   = chdr_header(rand64());
        ctrl_payload pyld = populate_ctrl_payload();

        memset(buff, 0, MAX_BUF_SIZE_BYTES);
        tx_pkt->refresh(buff, hdr, pyld);
        BOOST_CHECK(tx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(tx_pkt->get_payload() == pyld);

        byte_swap(buff);

        rx_pkt->refresh(buff);
        BOOST_CHECK(rx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(rx_pkt->get_payload() == pyld);
    }
}

BOOST_AUTO_TEST_CASE(chdr_strs_packet_no_swap_64)
{
    uint64_t buff[MAX_BUF_SIZE_WORDS];

    chdr_strs_packet::uptr tx_pkt  = chdr64_be_factory.make_strs();
    chdr_strs_packet::cuptr rx_pkt = chdr64_be_factory.make_strs();

    for (size_t i = 0; i < NUM_ITERS; i++) {
        chdr_header hdr   = chdr_header(rand64());
        strs_payload pyld = populate_strs_payload();

        memset(buff, 0, MAX_BUF_SIZE_BYTES);
        tx_pkt->refresh(buff, hdr, pyld);
        BOOST_CHECK(tx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(tx_pkt->get_payload() == pyld);

        rx_pkt->refresh(buff);
        BOOST_CHECK(rx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(rx_pkt->get_payload() == pyld);

        std::cout << pyld.to_string();
    }
}

BOOST_AUTO_TEST_CASE(chdr_strc_packet_no_swap_64)
{
    uint64_t buff[MAX_BUF_SIZE_WORDS];

    chdr_strc_packet::uptr tx_pkt  = chdr64_be_factory.make_strc();
    chdr_strc_packet::cuptr rx_pkt = chdr64_be_factory.make_strc();

    for (size_t i = 0; i < NUM_ITERS; i++) {
        chdr_header hdr   = chdr_header(rand64());
        strc_payload pyld = populate_strc_payload();

        memset(buff, 0, MAX_BUF_SIZE_BYTES);
        tx_pkt->refresh(buff, hdr, pyld);
        BOOST_CHECK(tx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(tx_pkt->get_payload() == pyld);

        rx_pkt->refresh(buff);
        BOOST_CHECK(rx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(rx_pkt->get_payload() == pyld);

        std::cout << pyld.to_string();
    }
}

BOOST_AUTO_TEST_CASE(chdr_generic_packet_calculate_pyld_offset_64)
{
    // Check calculation without timestamp
    auto test_pyld_offset = [](chdr_packet_writer::uptr& pkt,
                                const packet_type_t pkt_type,
                                const size_t num_mdata) {
        uint64_t buff[MAX_BUF_SIZE_WORDS];
        chdr_header header;
        header.set_pkt_type(pkt_type);
        header.set_num_mdata(num_mdata);

        pkt->refresh(reinterpret_cast<void*>(buff), header, 0);

        const size_t pyld_offset = pkt->calculate_payload_offset(pkt_type, num_mdata);

        void* pyld_ptr = pkt->get_payload_ptr();

        const size_t non_pyld_bytes = static_cast<size_t>(
            reinterpret_cast<uint8_t*>(pyld_ptr) - reinterpret_cast<uint8_t*>(buff));

        BOOST_CHECK(pyld_offset == non_pyld_bytes);
    };

    {
        chdr_packet_writer::uptr pkt = chdr64_be_factory.make_generic();
        test_pyld_offset(pkt, PKT_TYPE_DATA_NO_TS, 0);
        test_pyld_offset(pkt, PKT_TYPE_DATA_NO_TS, 1);
        test_pyld_offset(pkt, PKT_TYPE_DATA_NO_TS, 2);
        test_pyld_offset(pkt, PKT_TYPE_DATA_WITH_TS, 0);
        test_pyld_offset(pkt, PKT_TYPE_DATA_WITH_TS, 1);
        test_pyld_offset(pkt, PKT_TYPE_DATA_WITH_TS, 2);
    }
    {
        chdr_packet_writer::uptr pkt = chdr256_be_factory.make_generic();
        test_pyld_offset(pkt, PKT_TYPE_DATA_NO_TS, 0);
        test_pyld_offset(pkt, PKT_TYPE_DATA_NO_TS, 1);
        test_pyld_offset(pkt, PKT_TYPE_DATA_NO_TS, 2);
        test_pyld_offset(pkt, PKT_TYPE_DATA_WITH_TS, 0);
        test_pyld_offset(pkt, PKT_TYPE_DATA_WITH_TS, 1);
        test_pyld_offset(pkt, PKT_TYPE_DATA_WITH_TS, 2);
    }
}


BOOST_AUTO_TEST_CASE(chdr_mgmt_packet_no_swap_64)
{
    uint64_t buff[MAX_BUF_SIZE_WORDS];

    chdr_mgmt_packet::uptr tx_pkt  = chdr64_be_factory.make_mgmt();
    chdr_mgmt_packet::cuptr rx_pkt = chdr64_be_factory.make_mgmt();

    for (size_t i = 0; i < NUM_ITERS; i++) {
        chdr_header hdr   = chdr_header(rand64());
        mgmt_payload pyld = populate_mgmt_payload(CHDR_W_64);

        memset(buff, 0, MAX_BUF_SIZE_BYTES);
        tx_pkt->refresh(buff, hdr, pyld);
        BOOST_CHECK(tx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(tx_pkt->get_payload() == pyld);

        rx_pkt->refresh(buff);
        BOOST_CHECK(rx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(rx_pkt->get_payload() == pyld);

        std::cout << pyld.to_string();
    }
}

BOOST_AUTO_TEST_CASE(chdr_mgmt_packet_no_swap_256)
{
    uint64_t buff[MAX_BUF_SIZE_WORDS];

    chdr_mgmt_packet::uptr tx_pkt  = chdr256_be_factory.make_mgmt();
    chdr_mgmt_packet::cuptr rx_pkt = chdr256_be_factory.make_mgmt();

    for (size_t i = 0; i < NUM_ITERS; i++) {
        chdr_header hdr   = chdr_header(rand64());
        mgmt_payload pyld = populate_mgmt_payload(CHDR_W_256);

        memset(buff, 0, MAX_BUF_SIZE_BYTES);
        tx_pkt->refresh(buff, hdr, pyld);
        BOOST_CHECK(tx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(tx_pkt->get_payload() == pyld);

        rx_pkt->refresh(buff);
        BOOST_CHECK(rx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(rx_pkt->get_payload() == pyld);
    }
}

BOOST_AUTO_TEST_CASE(chdr_mgmt_packet_swap_64)
{
    uint64_t buff[MAX_BUF_SIZE_WORDS];

    chdr_mgmt_packet::uptr tx_pkt  = chdr64_be_factory.make_mgmt();
    chdr_mgmt_packet::cuptr rx_pkt = chdr64_le_factory.make_mgmt();

    for (size_t i = 0; i < NUM_ITERS; i++) {
        chdr_header hdr   = chdr_header(rand64());
        mgmt_payload pyld = populate_mgmt_payload(CHDR_W_64);

        memset(buff, 0, MAX_BUF_SIZE_BYTES);
        tx_pkt->refresh(buff, hdr, pyld);
        BOOST_CHECK(tx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(tx_pkt->get_payload() == pyld);

        byte_swap(buff);

        rx_pkt->refresh(buff);
        BOOST_CHECK(rx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(rx_pkt->get_payload() == pyld);
    }
}

BOOST_AUTO_TEST_CASE(chdr_mgmt_packet_swap_256)
{
    uint64_t buff[MAX_BUF_SIZE_WORDS];

    chdr_mgmt_packet::uptr tx_pkt  = chdr256_be_factory.make_mgmt();
    chdr_mgmt_packet::cuptr rx_pkt = chdr256_le_factory.make_mgmt();

    for (size_t i = 0; i < NUM_ITERS; i++) {
        chdr_header hdr   = chdr_header(rand64());
        mgmt_payload pyld = populate_mgmt_payload(CHDR_W_256);

        memset(buff, 0, MAX_BUF_SIZE_BYTES);
        tx_pkt->refresh(buff, hdr, pyld);
        BOOST_CHECK(tx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(tx_pkt->get_payload() == pyld);

        byte_swap(buff);

        rx_pkt->refresh(buff);
        BOOST_CHECK(rx_pkt->get_chdr_header() == hdr);
        BOOST_CHECK(rx_pkt->get_payload() == pyld);
    }
}

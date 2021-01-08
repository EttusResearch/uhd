//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/null_block_control.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

BOOST_AUTO_TEST_CASE(test_null_block)
{
    node_accessor_t node_accessor{};
    constexpr size_t num_chans    = 2;
    constexpr uint32_t nipc       = 2;
    constexpr uint32_t item_width = 32;
    constexpr noc_id_t noc_id     = 0x00000001;

    auto block_container = get_mock_block(noc_id, num_chans, num_chans);
    // Shorthand to save typing
    auto& reg_iface = block_container.reg_iface;
    auto set_mem    = [&](const uint32_t addr, const uint32_t data) {
        reg_iface->read_memory[addr] = data;
    };
    auto get_mem  = [&](const uint32_t addr) { return reg_iface->write_memory[addr]; };
    auto copy_mem = [&](const uint32_t addr) { set_mem(addr, get_mem(addr)); };
    set_mem(null_block_control::REG_CTRL_STATUS, (nipc << 24) | (item_width << 16));

    auto test_null = block_container.get_block<null_block_control>();
    BOOST_REQUIRE(test_null);

    using uhd::stream_cmd_t;
    node_accessor.init_props(test_null.get());
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    test_null->issue_stream_cmd(stream_cmd);
    stream_cmd.stream_mode = stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;
    BOOST_REQUIRE_THROW(test_null->issue_stream_cmd(stream_cmd), uhd::runtime_error);

    constexpr uint64_t snk_count      = 1000000000;
    constexpr uint64_t snk_count_pkts = 5;
    constexpr uint64_t src_count      = 2323232323;
    constexpr uint64_t loop_count     = 4242424242;
    set_mem(null_block_control::REG_SNK_LINE_CNT_LO,
        uhd::narrow_cast<uint32_t>(snk_count & 0xFFFFFFFF));
    set_mem(null_block_control::REG_SNK_LINE_CNT_HI,
        uhd::narrow_cast<uint32_t>((snk_count >> 32) & 0xFFFFFFFF));
    set_mem(null_block_control::REG_SNK_PKT_CNT_LO,
        uhd::narrow_cast<uint32_t>(snk_count_pkts & 0xFFFFFFFF));
    set_mem(null_block_control::REG_SNK_PKT_CNT_HI,
        uhd::narrow_cast<uint32_t>((snk_count_pkts >> 32) & 0xFFFFFFFF));
    set_mem(null_block_control::REG_SRC_LINE_CNT_LO,
        uhd::narrow_cast<uint32_t>(src_count & 0xFFFFFFFF));
    set_mem(null_block_control::REG_SRC_LINE_CNT_HI,
        uhd::narrow_cast<uint32_t>((src_count >> 32) & 0xFFFFFFFF));
    set_mem(null_block_control::REG_LOOP_LINE_CNT_LO,
        uhd::narrow_cast<uint32_t>(loop_count & 0xFFFFFFFF));
    set_mem(null_block_control::REG_LOOP_LINE_CNT_HI,
        uhd::narrow_cast<uint32_t>((loop_count >> 32) & 0xFFFFFFFF));
    BOOST_CHECK_EQUAL(
        test_null->get_count(null_block_control::SINK, null_block_control::LINES),
        snk_count);
    BOOST_CHECK_EQUAL(
        test_null->get_count(null_block_control::SINK, null_block_control::PACKETS),
        snk_count_pkts);
    BOOST_CHECK_EQUAL(
        test_null->get_count(null_block_control::SOURCE, null_block_control::LINES),
        src_count);
    BOOST_CHECK_EQUAL(
        test_null->get_count(null_block_control::LOOP, null_block_control::LINES),
        loop_count);

    constexpr uint32_t lpp = 3;
    constexpr uint32_t bpp = nipc * item_width / 8 * lpp;
    test_null->set_bytes_per_packet(bpp);
    copy_mem(null_block_control::REG_SRC_LINES_PER_PKT);
    copy_mem(null_block_control::REG_SRC_BYTES_PER_PKT);
    BOOST_CHECK_EQUAL(test_null->get_lines_per_packet(), lpp);
    BOOST_CHECK_EQUAL(test_null->get_bytes_per_packet(), bpp);

    auto sca = stream_cmd_action_info::make(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    node_accessor.send_action(test_null.get(), {res_source_info::OUTPUT_EDGE, 0}, sca);
    BOOST_CHECK_EQUAL(get_mem(null_block_control::REG_CTRL_STATUS) & 0x2, 0x2);
    BOOST_REQUIRE_THROW(node_accessor.send_action(
                            test_null.get(), {res_source_info::OUTPUT_EDGE, 1}, sca),
        uhd::runtime_error);
    BOOST_REQUIRE_THROW(
        node_accessor.send_action(test_null.get(), {res_source_info::INPUT_EDGE, 0}, sca),
        uhd::runtime_error);

    stream_cmd.stream_mode = stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
    test_null->issue_stream_cmd(stream_cmd);
    BOOST_CHECK_EQUAL(get_mem(null_block_control::REG_CTRL_STATUS) & 0x2, 0x2);
    node_accessor.shutdown(test_null.get());
    BOOST_CHECK_EQUAL(get_mem(null_block_control::REG_CTRL_STATUS) & 0x2, 0x0);
    test_null->issue_stream_cmd(stream_cmd);
    UHD_LOG_INFO("TEST", "Expected error message here ^^^");
    // The last issue_stream_cmd should do nothing b/c we called shutdown
    BOOST_CHECK_EQUAL(get_mem(null_block_control::REG_CTRL_STATUS) & 0x2, 0x0);
}

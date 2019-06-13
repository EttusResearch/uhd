//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rfnoc_mock_reg_iface.hpp"
#include <uhd/rfnoc/null_block_control.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/actions.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

namespace {

noc_block_base::make_args_ptr make_make_args(noc_block_base::noc_id_t noc_id,
    const std::string& block_id,
    const size_t n_inputs,
    const size_t n_outputs)
{
    auto make_args                = std::make_unique<noc_block_base::make_args_t>();
    make_args->noc_id             = noc_id;
    make_args->num_input_ports    = n_inputs;
    make_args->num_output_ports   = n_outputs;
    make_args->reg_iface          = std::make_shared<mock_reg_iface_t>();
    make_args->block_id           = block_id;
    make_args->ctrlport_clk_iface = std::make_shared<clock_iface>("MOCK_CLOCK");
    make_args->tb_clk_iface       = std::make_shared<clock_iface>("MOCK_CLOCK");
    make_args->tree               = uhd::property_tree::make();
    return make_args;
}

} // namespace

#define MOCK_REGISTER(BLOCK_NAME)                       \
    uhd::rfnoc::noc_block_base::sptr BLOCK_NAME##_make( \
        uhd::rfnoc::noc_block_base::make_args_ptr make_args);

MOCK_REGISTER(null_block_control)
MOCK_REGISTER(ddc_block_control)

BOOST_AUTO_TEST_CASE(test_null_block)
{
    node_accessor_t node_accessor{};
    constexpr size_t num_chans                 = 2;
    constexpr uint32_t nipc                    = 2;
    constexpr uint32_t item_width              = 32;
    constexpr noc_block_base::noc_id_t mock_id = 0x7E570000;

    auto make_args = make_make_args(mock_id, "0/NullSrcSink#0", num_chans, num_chans);
    auto reg_iface = std::dynamic_pointer_cast<mock_reg_iface_t>(make_args->reg_iface);
    auto set_mem   = [&](const uint32_t addr, const uint32_t data) {
        reg_iface->read_memory[addr] = data;
    };
    auto get_mem = [&](const uint32_t addr) { return reg_iface->write_memory[addr]; };
    auto copy_mem = [&](const uint32_t addr) { set_mem(addr, get_mem(addr)); };

    set_mem(null_block_control::REG_CTRL_STATUS, (nipc << 24) | (item_width << 16));
    auto test_null = std::dynamic_pointer_cast<null_block_control>(
        null_block_control_make(std::move(make_args)));

    using uhd::stream_cmd_t;
    node_accessor.init_props(test_null.get());
    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    test_null->issue_stream_cmd(stream_cmd);
    stream_cmd.stream_mode = stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE;
    BOOST_REQUIRE_THROW(test_null->issue_stream_cmd(stream_cmd), uhd::runtime_error);

    constexpr uint64_t snk_count = 1000000000;
    constexpr uint64_t snk_count_pkts = 5;
    constexpr uint64_t src_count = 2323232323;
    constexpr uint64_t loop_count = 4242424242;
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
    BOOST_REQUIRE_THROW(
        node_accessor.send_action(test_null.get(), {res_source_info::OUTPUT_EDGE, 1}, sca),
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

BOOST_AUTO_TEST_CASE(test_ddc_block)
{
    node_accessor_t node_accessor{};
    constexpr uint32_t num_hb                      = 2;
    constexpr uint32_t max_cic                     = 128;
    constexpr size_t num_chans                     = 4;
    constexpr noc_block_base::noc_id_t mock_noc_id = 0x7E57DDC0;

    auto ddc_make_args = make_make_args(mock_noc_id, "0/DDC#0", num_chans, num_chans);
    ddc_make_args->args = uhd::device_addr_t("foo=bar");
    auto ddc_reg_iface = std::dynamic_pointer_cast<mock_reg_iface_t>(ddc_make_args->reg_iface);
    ddc_reg_iface->read_memory[ddc_block_control::RB_COMPAT_NUM] =
        (ddc_block_control::MAJOR_COMPAT << 16) | ddc_block_control::MINOR_COMPAT;
    ddc_reg_iface->read_memory[ddc_block_control::RB_NUM_HB]        = num_hb;
    ddc_reg_iface->read_memory[ddc_block_control::RB_CIC_MAX_DECIM] = max_cic;
    auto test_ddc = ddc_block_control_make(std::move(ddc_make_args));
    BOOST_CHECK_EQUAL(test_ddc->get_block_args().get("foo"), "bar");

    node_accessor.init_props(test_ddc.get());
    UHD_LOG_DEBUG("TEST", "Init done.");
    test_ddc->set_property<int>("decim", 4, 0);

    BOOST_REQUIRE(ddc_reg_iface->write_memory.count(ddc_block_control::SR_DECIM_ADDR));
    BOOST_CHECK_EQUAL(
        ddc_reg_iface->write_memory.at(ddc_block_control::SR_DECIM_ADDR), 2 << 8 | 1);
}


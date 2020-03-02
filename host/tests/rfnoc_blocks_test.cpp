//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
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

namespace {

constexpr size_t DEFAULT_MTU = 8000;

} // namespace

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

BOOST_AUTO_TEST_CASE(test_ddc_block)
{
    node_accessor_t node_accessor{};
    constexpr uint32_t num_hb  = 2;
    constexpr uint32_t max_cic = 128;
    constexpr size_t num_chans = 4;
    constexpr noc_id_t noc_id  = DDC_BLOCK;
    constexpr int TEST_DECIM   = 20;

    auto block_container =
        get_mock_block(noc_id, num_chans, num_chans, uhd::device_addr_t("foo=bar"));
    auto& ddc_reg_iface = block_container.reg_iface;
    ddc_reg_iface->read_memory[ddc_block_control::RB_COMPAT_NUM] =
        (ddc_block_control::MAJOR_COMPAT << 16) | ddc_block_control::MINOR_COMPAT;
    ddc_reg_iface->read_memory[ddc_block_control::RB_NUM_HB]        = num_hb;
    ddc_reg_iface->read_memory[ddc_block_control::RB_CIC_MAX_DECIM] = max_cic;
    auto test_ddc = block_container.get_block<ddc_block_control>();
    BOOST_REQUIRE(test_ddc);
    BOOST_CHECK_EQUAL(test_ddc->get_block_args().get("foo"), "bar");

    node_accessor.init_props(test_ddc.get());
    UHD_LOG_DEBUG("TEST", "Init done.");
    test_ddc->set_property<int>("decim", TEST_DECIM, 0);

    BOOST_REQUIRE(ddc_reg_iface->write_memory.count(ddc_block_control::SR_DECIM_ADDR));
    BOOST_CHECK_EQUAL(
        ddc_reg_iface->write_memory.at(ddc_block_control::SR_DECIM_ADDR), 2 << 8 | 5);
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::INPUT_EDGE, 0}), DEFAULT_MTU);

    // Now plop it in a graph
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_info;
    edge_info.src_port                    = 0;
    edge_info.dst_port                    = 0;
    edge_info.property_propagation_active = true;
    edge_info.edge                        = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_terminator_t mock_source_term(1);
    mock_terminator_t mock_sink_term(1);

    UHD_LOG_INFO("TEST", "Priming mock source node props");
    mock_source_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<double>(
        "scaling", 1.0, {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<double>(
        "samp_rate", 1.0, {res_source_info::OUTPUT_EDGE, 0});
    constexpr size_t NEW_MTU = 4000;
    mock_source_term.set_edge_property<size_t>(
        "mtu", NEW_MTU, {res_source_info::OUTPUT_EDGE, 0});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_source_term, test_ddc.get(), edge_info);
    graph.connect(test_ddc.get(), &mock_sink_term, edge_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
    // We need to set the decimation again, because the rates will screw it
    // change it w.r.t. to the previous setting
    test_ddc->set_property<int>("decim", TEST_DECIM, 0);
    BOOST_CHECK_EQUAL(test_ddc->get_property<int>("decim", 0), TEST_DECIM);
    BOOST_CHECK(mock_source_term.get_edge_property<double>(
                    "samp_rate", {res_source_info::OUTPUT_EDGE, 0})
                == mock_sink_term.get_edge_property<double>(
                       "samp_rate", {res_source_info::INPUT_EDGE, 0})
                       * TEST_DECIM);
    BOOST_CHECK(mock_sink_term.get_edge_property<double>(
                    "scaling", {res_source_info::INPUT_EDGE, 0})
                != 1.0);

    UHD_LOG_INFO("TEST", "Setting freq to 1/8 of input rate");
    constexpr double TEST_FREQ = 1.0 / 8;
    test_ddc->set_property<double>("freq", TEST_FREQ, 0);
    const uint32_t freq_word_1 =
        ddc_reg_iface->write_memory.at(ddc_block_control::SR_FREQ_ADDR);
    BOOST_REQUIRE(freq_word_1 != 0);
    UHD_LOG_INFO("TEST", "Doubling input rate (to 2.0)");
    // Now this should change the freq word, but not the absolute frequency
    mock_source_term.set_edge_property<double>(
        "samp_rate", 2.0, {res_source_info::OUTPUT_EDGE, 0});
    const double freq_word_2 =
        ddc_reg_iface->write_memory.at(ddc_block_control::SR_FREQ_ADDR);
    // The frequency word is the phase increment, which will halve. We skirt
    // around fixpoint/floating point accuracy issues by using CLOSE.
    BOOST_CHECK_CLOSE(double(freq_word_1) / double(freq_word_2), 2.0, 1e-6);

    UHD_LOG_INFO("TEST", "Testing DDC MTU propagation");
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::INPUT_EDGE, 0}), NEW_MTU);
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::OUTPUT_EDGE, 0}), NEW_MTU);
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::INPUT_EDGE, 1}), DEFAULT_MTU);
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::OUTPUT_EDGE, 1}), DEFAULT_MTU);
    mock_source_term.set_edge_property<size_t>(
        "mtu", NEW_MTU / 2, {res_source_info::OUTPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::INPUT_EDGE, 0}), NEW_MTU / 2);
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::OUTPUT_EDGE, 0}), NEW_MTU / 2);

    // Now reset the props using set_properties
    test_ddc->set_properties(uhd::device_addr_t("decim=1,freq=0.0,foo=bar"), 0);
    BOOST_CHECK_EQUAL(test_ddc->get_property<int>("decim", 0), 1);
    BOOST_CHECK_EQUAL(test_ddc->get_property<double>("freq", 0), 0.0);
}

BOOST_AUTO_TEST_CASE(test_duc_block)
{
    node_accessor_t node_accessor{};
    constexpr uint32_t num_hb  = 2;
    constexpr uint32_t max_cic = 128;
    constexpr size_t num_chans = 4;
    constexpr noc_id_t noc_id  = DUC_BLOCK;
    constexpr int TEST_INTERP  = 20; // 2 halfbands, CIC==5

    auto block_container = get_mock_block(noc_id, num_chans, num_chans);
    auto& duc_reg_iface  = block_container.reg_iface;
    duc_reg_iface->read_memory[duc_block_control::RB_COMPAT_NUM] =
        (duc_block_control::MAJOR_COMPAT << 16) | duc_block_control::MINOR_COMPAT;
    duc_reg_iface->read_memory[duc_block_control::RB_NUM_HB]         = num_hb;
    duc_reg_iface->read_memory[duc_block_control::RB_CIC_MAX_INTERP] = max_cic;
    auto test_duc = block_container.get_block<duc_block_control>();
    BOOST_REQUIRE(test_duc);

    node_accessor.init_props(test_duc.get());
    UHD_LOG_DEBUG("TEST", "Init done.");
    test_duc->set_property<int>("interp", TEST_INTERP, 0);

    BOOST_REQUIRE(duc_reg_iface->write_memory.count(duc_block_control::SR_INTERP_ADDR));
    BOOST_CHECK_EQUAL(
        duc_reg_iface->write_memory.at(duc_block_control::SR_INTERP_ADDR), 2 << 8 | 5);
    BOOST_CHECK_EQUAL(test_duc->get_mtu({res_source_info::INPUT_EDGE, 0}), DEFAULT_MTU);

    // Now plop it in a graph
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_info;
    edge_info.src_port                    = 0;
    edge_info.dst_port                    = 0;
    edge_info.property_propagation_active = true;
    edge_info.edge                        = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_terminator_t mock_source_term(1, {ACTION_KEY_STREAM_CMD});
    mock_terminator_t mock_sink_term(1, {ACTION_KEY_STREAM_CMD});

    UHD_LOG_INFO("TEST", "Priming mock source node props");
    mock_source_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<double>(
        "scaling", 1.0, {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<double>(
        "samp_rate", 1.0, {res_source_info::OUTPUT_EDGE, 0});
    UHD_LOG_INFO("TEST", "Priming mock sink node props");
    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 0});
    mock_sink_term.set_edge_property<double>(
        "scaling", 1.0, {res_source_info::INPUT_EDGE, 0});
    mock_sink_term.set_edge_property<double>(
        "samp_rate", 1.0, {res_source_info::INPUT_EDGE, 0});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_source_term, test_duc.get(), edge_info);
    graph.connect(test_duc.get(), &mock_sink_term, edge_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
    // We need to set the interpation again, because the rates will screw it
    // change it w.r.t. to the previous setting
    test_duc->set_property<int>("interp", TEST_INTERP, 0);
    BOOST_CHECK_EQUAL(test_duc->get_property<int>("interp", 0), TEST_INTERP);
    BOOST_CHECK(mock_source_term.get_edge_property<double>(
                    "samp_rate", {res_source_info::OUTPUT_EDGE, 0})
                    * TEST_INTERP
                == mock_sink_term.get_edge_property<double>(
                       "samp_rate", {res_source_info::INPUT_EDGE, 0}));
    const double initial_input_scaling = mock_source_term.get_edge_property<double>(
        "scaling", {res_source_info::OUTPUT_EDGE, 0});
    const double initial_output_scaling = mock_sink_term.get_edge_property<double>(
        "scaling", {res_source_info::INPUT_EDGE, 0});
    // Our chosen interpolation value will cause some scaling issues, so
    // this value needs to be off from 1.0
    BOOST_CHECK(initial_input_scaling != 1.0);
    BOOST_CHECK(initial_output_scaling == 1.0);

    // The DUC will not let us set the scaling on its input, so the following
    // call to set property should have no effect
    mock_source_term.set_edge_property<double>(
        "scaling", 42.0, {res_source_info::OUTPUT_EDGE, 0});
    BOOST_CHECK(initial_input_scaling
                == mock_source_term.get_edge_property<double>(
                       "scaling", {res_source_info::OUTPUT_EDGE, 0}));
    BOOST_CHECK(initial_output_scaling
                == mock_sink_term.get_edge_property<double>(
                       "scaling", {res_source_info::INPUT_EDGE, 0}));
    // However, if we change the scaling on the DUC's output, that will
    // propagate to its input
    UHD_LOG_INFO("TEST", "Testing doubling the output scaling...");
    mock_sink_term.set_edge_property<double>(
        "scaling", 2.0, {res_source_info::INPUT_EDGE, 0});
    const double doubled_input_scaling = mock_source_term.get_edge_property<double>(
        "scaling", {res_source_info::OUTPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(doubled_input_scaling, 2 * initial_input_scaling);

    UHD_LOG_INFO("TEST", "Setting freq to 1/8 of input rate");
    constexpr double TEST_FREQ = 1.0 / 8;
    test_duc->set_property<double>("freq", TEST_FREQ, 0);
    const uint32_t freq_word_1 =
        duc_reg_iface->write_memory.at(duc_block_control::SR_FREQ_ADDR);
    BOOST_REQUIRE(freq_word_1 != 0);
    UHD_LOG_INFO("TEST", "Doubling input rate (to 2.0)");
    // Now this should change the freq word, but not the absolute frequency
    mock_sink_term.set_edge_property<double>("samp_rate",
        2
            * mock_sink_term.get_edge_property<double>(
                  "samp_rate", {res_source_info::INPUT_EDGE, 0}),
        {res_source_info::INPUT_EDGE, 0});
    const double freq_word_2 =
        duc_reg_iface->write_memory.at(duc_block_control::SR_FREQ_ADDR);
    // The frequency word is the phase increment, which will halve. We skirt
    // around fixpoint/floating point accuracy issues by using CLOSE.
    BOOST_CHECK_CLOSE(double(freq_word_1) / double(freq_word_2), 2.0, 1e-6);

    // Reset the interpolation
    test_duc->set_property<int>("interp", TEST_INTERP, 0);
    BOOST_REQUIRE_EQUAL(test_duc->get_property<int>("interp", 0), TEST_INTERP);
    UHD_LOG_INFO("TEST", "DUC: Testing action forwarding");
    auto new_stream_cmd_action =
        stream_cmd_action_info::make(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    new_stream_cmd_action->stream_cmd.num_samps = 1000;
    node_accessor.post_action(
        &mock_sink_term, {res_source_info::INPUT_EDGE, 0}, new_stream_cmd_action);
    BOOST_REQUIRE(!mock_source_term.received_actions.empty());
    auto stream_cmd_recv_by_src = std::dynamic_pointer_cast<stream_cmd_action_info>(
        mock_source_term.received_actions.back());
    BOOST_CHECK(stream_cmd_recv_by_src);
    BOOST_CHECK_EQUAL(stream_cmd_recv_by_src->stream_cmd.num_samps, 1000 / TEST_INTERP);
    auto new_stream_cmd_action2 =
        stream_cmd_action_info::make(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    node_accessor.post_action(
        &mock_sink_term, {res_source_info::INPUT_EDGE, 0}, new_stream_cmd_action2);
    BOOST_REQUIRE(!mock_source_term.received_actions.empty());
    auto stream_cmd_recv_by_src2 = std::dynamic_pointer_cast<stream_cmd_action_info>(
        mock_source_term.received_actions.back());
    BOOST_CHECK_EQUAL(stream_cmd_recv_by_src2->stream_cmd.stream_mode,
        uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    auto new_stream_cmd_action3 =
        stream_cmd_action_info::make(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    new_stream_cmd_action3->stream_cmd.num_samps = 100;
    node_accessor.post_action(
        &mock_source_term, {res_source_info::OUTPUT_EDGE, 0}, new_stream_cmd_action3);
    BOOST_REQUIRE(!mock_sink_term.received_actions.empty());
    auto stream_cmd_recv_by_src3 = std::dynamic_pointer_cast<stream_cmd_action_info>(
        mock_sink_term.received_actions.back());
    BOOST_CHECK(stream_cmd_recv_by_src3);
    BOOST_CHECK_EQUAL(stream_cmd_recv_by_src3->stream_cmd.num_samps, 100 * TEST_INTERP);
}

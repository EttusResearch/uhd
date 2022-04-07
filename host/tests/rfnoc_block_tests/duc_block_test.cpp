//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/mock_block.hpp>
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

BOOST_AUTO_TEST_CASE(test_duc_block)
{
    node_accessor_t node_accessor{};
    constexpr uint32_t num_hb  = 2;
    constexpr uint32_t max_cic = 128;
    constexpr size_t num_chans = 4;
    constexpr noc_id_t noc_id  = DUC_BLOCK;
    constexpr int TEST_INTERP  = 20; // 2 halfbands, CIC==5
    constexpr double DEFAULT_RATE = 200e6; // Matches typical MCR of X310

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
    edge_info.src_port        = 0;
    edge_info.dst_port        = 0;
    edge_info.is_forward_edge = true;
    edge_info.edge            = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_terminator_t mock_source_term(1, {ACTION_KEY_STREAM_CMD});
    mock_terminator_t mock_sink_term(1, {ACTION_KEY_STREAM_CMD});

    UHD_LOG_INFO("TEST", "Priming mock source node props");
    mock_source_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<double>(
        "scaling", 1.0, {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<double>(
        "samp_rate", DEFAULT_RATE / TEST_INTERP, {res_source_info::OUTPUT_EDGE, 0});
    UHD_LOG_INFO("TEST", "Priming mock sink node props");
    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 0});
    mock_sink_term.set_edge_property<double>(
        "scaling", 1.0, {res_source_info::INPUT_EDGE, 0});
    mock_sink_term.set_edge_property<double>(
        "samp_rate", DEFAULT_RATE, {res_source_info::INPUT_EDGE, 0});

#define CHECK_OUTPUT_RATE(req_rate)                                         \
    BOOST_REQUIRE_CLOSE(mock_sink_term.get_edge_property<double>(           \
                            "samp_rate", {res_source_info::INPUT_EDGE, 0}), \
        req_rate,                                                           \
        1e-6);


    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_source_term, test_duc.get(), edge_info);
    graph.connect(test_duc.get(), &mock_sink_term, edge_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
    CHECK_OUTPUT_RATE(DEFAULT_RATE);
    // We need to set the interpation again, because the rates will screw it
    // change it w.r.t. to the previous setting
    test_duc->set_property<int>("interp", TEST_INTERP, 0);
    BOOST_CHECK_EQUAL(test_duc->get_property<int>("interp", 0), TEST_INTERP);
    BOOST_CHECK(mock_source_term.get_edge_property<double>(
                    "samp_rate", {res_source_info::OUTPUT_EDGE, 0})
                    * TEST_INTERP
                == mock_sink_term.get_edge_property<double>(
                       "samp_rate", {res_source_info::INPUT_EDGE, 0}));
    // Output rate should remain unchanged
    CHECK_OUTPUT_RATE(DEFAULT_RATE);
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

    BOOST_CHECK_CLOSE(test_duc->get_frequency_range(0).start(), -DEFAULT_RATE / 2, 1e-6);
    BOOST_CHECK_CLOSE(test_duc->get_frequency_range(0).stop(), DEFAULT_RATE / 2, 1e-6);
    UHD_LOG_INFO("TEST",
        "Setting freq to 1/8 of input rate (to " << (DEFAULT_RATE / 8) / 1e6 << " MHz)");
    constexpr double TEST_FREQ = DEFAULT_RATE / 8;
    test_duc->set_property<double>("freq", TEST_FREQ, 0);
    const uint32_t freq_word_1 =
        duc_reg_iface->write_memory.at(duc_block_control::SR_FREQ_ADDR);
    BOOST_REQUIRE(freq_word_1 != 0);
    UHD_LOG_INFO(
        "TEST", "Doubling input rate (to " << (DEFAULT_RATE / 4) / 1e6 << " MHz)");
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

//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/node.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/rfnoc/prop_accessor.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>


const std::string STREAM_CMD_KEY = "stream_cmd";

BOOST_AUTO_TEST_CASE(test_actions_single_node)
{
    node_accessor_t node_accessor{};

    // Define some mock nodes:
    mock_radio_node_t mock_radio(0);

    auto stream_cmd =
        stream_cmd_action_info::make(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    auto other_cmd = action_info::make("FOO");

    node_accessor.send_action(&mock_radio, {res_source_info::INPUT_EDGE, 0}, stream_cmd);
    node_accessor.send_action(&mock_radio, {res_source_info::INPUT_EDGE, 0}, other_cmd);

    mock_radio.update_fwd_policy(node_t::forwarding_policy_t::ONE_TO_ONE);
    node_accessor.send_action(&mock_radio, {res_source_info::INPUT_EDGE, 0}, other_cmd);
    mock_radio.update_fwd_policy(node_t::forwarding_policy_t::ONE_TO_FAN);
    node_accessor.send_action(&mock_radio, {res_source_info::INPUT_EDGE, 0}, other_cmd);
    mock_radio.update_fwd_policy(node_t::forwarding_policy_t::ONE_TO_ALL);
    node_accessor.send_action(&mock_radio, {res_source_info::INPUT_EDGE, 0}, other_cmd);
    mock_radio.update_fwd_policy(node_t::forwarding_policy_t::ONE_TO_ALL_IN);
    node_accessor.send_action(&mock_radio, {res_source_info::INPUT_EDGE, 0}, other_cmd);
    mock_radio.update_fwd_policy(node_t::forwarding_policy_t::ONE_TO_ALL_OUT);
    node_accessor.send_action(&mock_radio, {res_source_info::INPUT_EDGE, 0}, other_cmd);

    uhd::rfnoc::detail::graph_t graph{};
    graph.connect(&mock_radio, &mock_radio, {0, 0, graph_edge_t::DYNAMIC, false});
    graph.commit();
    stream_cmd =
        stream_cmd_action_info::make(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd->stream_cmd.num_samps = 37;
    node_accessor.post_action(&mock_radio, {res_source_info::USER, 0}, stream_cmd);
    BOOST_REQUIRE_EQUAL(mock_radio.last_num_samps, 37);
}

BOOST_AUTO_TEST_CASE(test_actions_simple_graph)
{
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};

    // Define some mock nodes:
    mock_radio_node_t mock_rx_radio{0};
    mock_ddc_node_t mock_ddc{};
    mock_fifo_t mock_fifo{1};
    mock_streamer_t mock_streamer{1};

    // These init calls would normally be done by the framework
    node_accessor.init_props(&mock_rx_radio);
    node_accessor.init_props(&mock_ddc);
    node_accessor.init_props(&mock_fifo);
    node_accessor.init_props(&mock_streamer);

    graph.connect(&mock_rx_radio, &mock_ddc, {0, 0, graph_edge_t::DYNAMIC, true});
    graph.connect(&mock_ddc, &mock_fifo, {0, 0, graph_edge_t::DYNAMIC, true});
    graph.connect(&mock_fifo, &mock_streamer, {0, 0, graph_edge_t::DYNAMIC, true});
    graph.commit();

    // Force the DDC to actually set a decimation rate != 1
    mock_streamer.set_property<double>("samp_rate", 10e6, 0);

    uhd::stream_cmd_t num_samps_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    constexpr size_t NUM_SAMPS = 100;
    num_samps_cmd.num_samps    = NUM_SAMPS;

    mock_streamer.issue_stream_cmd(num_samps_cmd, 0);
    BOOST_CHECK_EQUAL(
        NUM_SAMPS * mock_ddc.get_property<int>("decim", 0), mock_rx_radio.last_num_samps);
}

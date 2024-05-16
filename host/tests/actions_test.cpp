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

BOOST_AUTO_TEST_CASE(test_action_forwarding_map)
{
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};

    constexpr size_t NUM_GENERATOR_OUTPUTS = 8;
    constexpr size_t NUM_RECEIVER_INPUTS   = 8;

    node_t::forwarding_map_t fwd_map = {
        // input edges 0-3 --> output edges 3-0
        {{res_source_info::INPUT_EDGE, 0}, {{res_source_info::OUTPUT_EDGE, 3}}},
        {{res_source_info::INPUT_EDGE, 1}, {{res_source_info::OUTPUT_EDGE, 2}}},
        {{res_source_info::INPUT_EDGE, 2}, {{res_source_info::OUTPUT_EDGE, 1}}},
        {{res_source_info::INPUT_EDGE, 3}, {{res_source_info::OUTPUT_EDGE, 0}}},
        // input edge 4 --> output edges 4 and 5
        {{res_source_info::INPUT_EDGE, 4},
            {{res_source_info::OUTPUT_EDGE, 4}, {res_source_info::OUTPUT_EDGE, 5}}},
        // input edge 5 --> output edges 6 and 7
        {{res_source_info::INPUT_EDGE, 5},
            {{res_source_info::OUTPUT_EDGE, 6}, {res_source_info::OUTPUT_EDGE, 7}}},
        // input edge 6 no destination (i.e. drop)
        {{res_source_info::INPUT_EDGE, 6}, {}}
        // input edge 7 not in map (i.e. drop)
    };

    mock_edge_node_t generator{0, NUM_GENERATOR_OUTPUTS, "MOCK_EDGE_NODE<generator>"};
    mock_routing_node_t router{NUM_GENERATOR_OUTPUTS, NUM_RECEIVER_INPUTS};
    mock_edge_node_t receiver{NUM_RECEIVER_INPUTS, 0, "MOCK_EDGE_NODE<receiver>"};

    router.set_action_forwarding_map(fwd_map);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&generator);
    node_accessor.init_props(&router);
    node_accessor.init_props(&receiver);

    using graph_edge_t = uhd::rfnoc::detail::graph_t::graph_edge_t;

    // Connect the nodes in the graph
    for (size_t i = 0; i < NUM_GENERATOR_OUTPUTS; i++) {
        graph.connect(&generator, &router, {i, i, graph_edge_t::DYNAMIC, true});
    }
    for (size_t i = 0; i < NUM_RECEIVER_INPUTS; i++) {
        graph.connect(&router, &receiver, {i, i, graph_edge_t::DYNAMIC, true});
    }
    graph.commit();

    UHD_LOG_INFO("TEST", "Now testing map-driven action forwarding");
    for (size_t i = 0; i < NUM_GENERATOR_OUTPUTS; i++) {
        auto cmd = action_info::make("action");
        // The payload of the outgoing event sent from the generator
        // consists of the port number on which the action was emitted.
        // This makes it easier for us to ensure that the action was routed
        // to the correct receiver port, since the actions are all named
        // 'action'.
        cmd->payload.push_back(static_cast<uint8_t>(i));
        generator.post_output_edge_action(cmd, i);
    }

    auto received_actions               = receiver.get_received_actions_map();
    auto receiver_port_get_action_count = [received_actions](size_t port) -> size_t {
        auto itr = received_actions.find({res_source_info::INPUT_EDGE, port});
        if (itr == received_actions.end()) {
            return 0;
        }
        return itr->second.size();
    };
    auto receiver_port_get_action = [received_actions](
                                        size_t port, size_t n) -> action_info::sptr {
        auto itr = received_actions.find({res_source_info::INPUT_EDGE, port});
        if (itr == received_actions.end()) {
            // If the action wasn't found, return a dummy action
            auto bad_action = action_info::make("invalid_action");
            return bad_action;
        }
        return itr->second.at(n);
    };

    // Ensure correct count of actions at each receiver port
    BOOST_CHECK_EQUAL(receiver_port_get_action_count(0), 1);
    BOOST_CHECK_EQUAL(receiver_port_get_action_count(1), 1);
    BOOST_CHECK_EQUAL(receiver_port_get_action_count(2), 1);
    BOOST_CHECK_EQUAL(receiver_port_get_action_count(3), 1);
    BOOST_CHECK_EQUAL(receiver_port_get_action_count(4), 1);
    BOOST_CHECK_EQUAL(receiver_port_get_action_count(5), 1);
    BOOST_CHECK_EQUAL(receiver_port_get_action_count(6), 1);
    BOOST_CHECK_EQUAL(receiver_port_get_action_count(7), 1);

    // Ensure correct payload of received action (indicates source edge from
    // which the action was generator)
    BOOST_CHECK_EQUAL(receiver_port_get_action(0, 0)->payload.at(0), 3);
    BOOST_CHECK_EQUAL(receiver_port_get_action(1, 0)->payload.at(0), 2);
    BOOST_CHECK_EQUAL(receiver_port_get_action(2, 0)->payload.at(0), 1);
    BOOST_CHECK_EQUAL(receiver_port_get_action(3, 0)->payload.at(0), 0);
    BOOST_CHECK_EQUAL(receiver_port_get_action(4, 0)->payload.at(0), 4);
    BOOST_CHECK_EQUAL(receiver_port_get_action(5, 0)->payload.at(0), 4);
    BOOST_CHECK_EQUAL(receiver_port_get_action(6, 0)->payload.at(0), 5);
    BOOST_CHECK_EQUAL(receiver_port_get_action(7, 0)->payload.at(0), 5);
}

BOOST_AUTO_TEST_CASE(test_action_forwarding_map_exception_invalid_destination)
{
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};

    // Create a map that will generate an exception at action forwarding time
    // due to the mapping pointing to a non-existent port
    node_t::forwarding_map_t invalid_fwd_map = {
        // input edge 0 --> output edge 1 (output port does not exist)
        {{res_source_info::INPUT_EDGE, 0}, {{res_source_info::OUTPUT_EDGE, 1}}},
    };

    mock_edge_node_t generator{0, 1, "MOCK_EDGE_NODE<generator>"};
    mock_routing_node_t router{1, 1};
    mock_edge_node_t receiver{1, 0, "MOCK_EDGE_NODE<receiver>"};

    router.set_action_forwarding_map(invalid_fwd_map);

    // These init calls would normally be done by the framework
    node_accessor.init_props(&generator);
    node_accessor.init_props(&router);
    node_accessor.init_props(&receiver);

    using graph_edge_t = uhd::rfnoc::detail::graph_t::graph_edge_t;

    // Connect the nodes in the graph
    graph.connect(&generator, &router, {0, 0, graph_edge_t::DYNAMIC, true});
    graph.connect(&router, &receiver, {0, 0, graph_edge_t::DYNAMIC, true});
    graph.commit();

    UHD_LOG_INFO(
        "TEST", "Now testing action forwarding with invalid map (no destination port)");
    auto cmd = action_info::make("action");
    BOOST_REQUIRE_THROW(generator.post_output_edge_action(cmd, 0), uhd::rfnoc_error);
}

BOOST_AUTO_TEST_CASE(test_action_exception_handling)
{
    node_accessor_t node_accessor{};
    uhd::rfnoc::detail::graph_t graph{};

    class mock_throwing_node_t : public mock_radio_node_t
    {
    public:
        mock_throwing_node_t() : mock_radio_node_t(0)
        {
            register_action_handler(
                "throwing_action", [](const res_source_info&, action_info::sptr) {
                    throw uhd::runtime_error("Arbitrary UHD exception");
                });
        }
    };

    mock_throwing_node_t mock_radio{};
    graph.connect(&mock_radio, &mock_radio, {0, 0, graph_edge_t::DYNAMIC, false});
    graph.commit();
    // Check that it throws the first time
    BOOST_REQUIRE_THROW(node_accessor.post_action(&mock_radio,
                            {res_source_info::USER, 0},
                            action_info::make("throwing_action")),
        uhd::runtime_error);
    // It should also throw the second time: we should actually be running this action
    // even though the previous one threw an exception
    BOOST_REQUIRE_THROW(node_accessor.post_action(&mock_radio,
                            {res_source_info::USER, 0},
                            action_info::make("throwing_action")),
        uhd::runtime_error);
}

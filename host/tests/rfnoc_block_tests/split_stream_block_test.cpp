//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/split_stream_block_control.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

struct split_stream_block_fixture
{
    //! Create a split stream block having a particular number of input and
    //  and output ports.
    void create_block(size_t num_inputs, size_t num_outputs)
    {
        num_input_ports  = num_inputs;
        num_output_ports = num_outputs;
        block_container  = get_mock_block(
            SPLIT_STREAM_BLOCK, num_input_ports, num_output_ports, uhd::device_addr_t());
        test_split_stream = block_container.get_block<split_stream_block_control>();
        node_accessor.init_props(test_split_stream.get());
    }

    //! Connect one source and N sinks, one per branch, to the split stream
    //  block.
    void connect_graph(detail::graph_t& graph,
        mock_edge_node_t& source,
        const std::vector<std::unique_ptr<mock_edge_node_t>>& sinks)
    {
        // The mock source must have the same number of output ports as the
        // split stream block has inputs.
        UHD_ASSERT_THROW(source.get_num_output_ports() == num_input_ports);
        // The caller must provide one sink for each output branch of the
        // split stream block.
        UHD_ASSERT_THROW(sinks.size() == num_output_ports / num_input_ports);

        // Create a graph with the split stream block in between a mock source
        // and sink, and set the initial edge properties for the source's
        // ports.
        UHD_LOG_INFO("TEST", "Priming mock source node props");
        for (size_t port = 0; port < num_input_ports; port++) {
            source.set_edge_property<int>(
                "prop", 1000 * port, {res_source_info::OUTPUT_EDGE, port});
        }

        // Commit the graph.
        UHD_LOG_INFO("TEST", "Creating graph...");
        // Connect source ports to split stream input ports.
        for (size_t port = 0; port < num_input_ports; port++) {
            graph.connect(&source,
                test_split_stream.get(),
                graph_edge_t{port, port, detail::graph_t::graph_edge_t::DYNAMIC, true});
        }
        // For each output branch of the split stream block...
        for (size_t branch = 0; branch < sinks.size(); branch++) {
            UHD_ASSERT_THROW(sinks.at(branch)->get_num_input_ports() == num_input_ports);
            // ...and for each output port...
            for (size_t port = 0; port < num_input_ports; port++) {
                // ...connect the output ports of the branch to the input
                // ports of the sink connected to that branch
                graph.connect(test_split_stream.get(),
                    sinks.at(branch).get(),
                    graph_edge_t{branch * num_input_ports + port,
                        port,
                        detail::graph_t::graph_edge_t::DYNAMIC,
                        true});
            }
        }
        UHD_LOG_INFO("TEST", "Committing graph...");
        graph.commit();
        UHD_LOG_INFO("TEST", "Commit complete.");
    }

    size_t num_input_ports  = 0;
    size_t num_output_ports = 0;
    mock_block_container block_container{};
    std::shared_ptr<split_stream_block_control> test_split_stream{};
    node_accessor_t node_accessor{};
};

/*
 * This test case creates split stream blocks with different (valid) input
 * and output ports, and makes sure properties and actions are handled
 * appropriately.
 */
BOOST_FIXTURE_TEST_CASE(test_split_stream_block, split_stream_block_fixture)
{
    const std::vector<std::pair<size_t, size_t>> split_stream_configs{
        // Pairs are {number of inputs, number of outputs}
        {1, 2},
        {2, 4},
        {2, 8},
        {4, 20}};

    // Create a graph using the split stream block.
    for (const auto& config : split_stream_configs) {
        const size_t num_inputs   = config.first;
        const size_t num_outputs  = config.second;
        const size_t num_branches = num_outputs / num_inputs;
        create_block(num_inputs, num_outputs);

        detail::graph_t graph{};

        mock_edge_node_t source{0, num_inputs, "MOCK_SOURCE"};
        std::vector<std::unique_ptr<mock_edge_node_t>> sinks;
        for (size_t branch = 0; branch < num_branches; branch++) {
            std::string sink_name = "MOCK_SINK<" + std::to_string(branch) + ">";
            auto sink = std::make_unique<mock_edge_node_t>(num_inputs, 0, sink_name);
            sinks.push_back(std::move(sink));
        }

        connect_graph(graph, source, sinks);

        // Check that 'prop' is propagated as expected from each stream of
        // the input branch to the corresponding stream of all output
        // branches.
        UHD_LOG_INFO("TEST", "Testing initial property propagation...");
        for (size_t branch = 0; branch < num_branches; branch++) {
            for (size_t port = 0; port < num_inputs; port++) {
                int prop_val = sinks.at(branch)->get_edge_property<int>(
                    "prop", {res_source_info::INPUT_EDGE, port});
                int expected_prop_val = port * 1000;
                BOOST_CHECK_EQUAL(prop_val, expected_prop_val);
            }
        }

        // Change the 'prop' property on each stream of each output branch.
        // Ensure that the value gets propagated to the correct input branch
        // stream port and the corresponding stream port of all other
        // output branches.
        UHD_LOG_INFO("TEST", "Testing property propagation...");
        for (size_t branch = 0; branch < num_branches; branch++) {
            for (size_t port = 0; port < num_inputs; port++) {
                const int new_prop_val = (100 * branch) + port;

                sinks.at(branch)->set_edge_property<int>(
                    "prop", new_prop_val, {res_source_info::INPUT_EDGE, port});
                BOOST_CHECK_EQUAL(source.get_edge_property<int>(
                                      "prop", {res_source_info::OUTPUT_EDGE, port}),
                    new_prop_val);

                for (size_t other_branch = 0; other_branch < num_branches;
                     other_branch++) {
                    if (other_branch == branch)
                        continue;

                    BOOST_CHECK_EQUAL(sinks.at(other_branch)
                                          ->get_edge_property<int>("prop",
                                              {res_source_info::INPUT_EDGE, port}),
                        new_prop_val);
                }
            }
        }

        // Check that an action originating on an input stream is forwarded to
        // the port corresponding to that stream on all output branches.
        UHD_LOG_INFO("TEST", "Testing action forwarding from input branch...");
        for (size_t port = 0; port < num_inputs; port++) {
            auto cmd = action_info::make("action");
            cmd->payload.push_back(static_cast<uint8_t>(port));
            source.post_output_edge_action(cmd, port);
        }

        for (size_t branch = 0; branch < num_branches; branch++) {
            for (size_t port = 0; port < num_inputs; port++) {
                auto received_actions = sinks.at(branch)->get_received_actions_map();
                res_source_info port_to_check{res_source_info::INPUT_EDGE, port};
                BOOST_CHECK_EQUAL(received_actions.count(port_to_check), 1);
                BOOST_CHECK_EQUAL(
                    received_actions.find(port_to_check)->second.at(0)->payload.at(0),
                    port);
            }
        }

        // Now check that an action originating on an output branch is
        // forwarded back to the appropriate input stream.
        UHD_LOG_INFO("TEST", "Testing action forwarding from output branches...");
        for (size_t branch = 0; branch < num_branches; branch++) {
            // Clear the received actions map on the source so that we've
            // effectively got a clean slate for each of the output branches
            // under test.
            source.clear_received_actions_map();

            for (size_t port = 0; port < num_inputs; port++) {
                auto cmd = action_info::make("action");
                cmd->payload.push_back(static_cast<uint8_t>(branch));
                cmd->payload.push_back(static_cast<uint8_t>(port));
                sinks.at(branch)->post_input_edge_action(cmd, port);

                auto received_actions = source.get_received_actions_map();
                res_source_info port_to_check{res_source_info::OUTPUT_EDGE, port};
                BOOST_CHECK_EQUAL(received_actions.count(port_to_check), 1);
                BOOST_CHECK_EQUAL(
                    received_actions.find(port_to_check)->second.at(0)->payload.at(0),
                    branch);
                BOOST_CHECK_EQUAL(
                    received_actions.find(port_to_check)->second.at(0)->payload.at(1),
                    port);
            }
        }
    }
}

/*
 * This test case ensures that the split stream block will throw an assertion
 * if it is incorrectly configured.
 */
BOOST_FIXTURE_TEST_CASE(test_invalid_port_count, split_stream_block_fixture)
{
    // Invalid case: Output ports not an integer multiple of input ports.
    BOOST_CHECK_THROW(create_block(2, 5), uhd::assertion_error);
    BOOST_CHECK_THROW(create_block(4, 19), uhd::assertion_error);

    // Invalid case: Split stream block with only one branch (why?)
    BOOST_CHECK_THROW(create_block(1, 1), uhd::assertion_error);
    BOOST_CHECK_THROW(create_block(4, 4), uhd::assertion_error);
}

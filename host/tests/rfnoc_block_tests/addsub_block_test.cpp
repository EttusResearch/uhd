//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/addsub_block_control.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

/* addsub_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container and addsub_block_control
 * object, all of which are accessible to the test case. The instance of the
 * object is destroyed at the end of each test case.
 */
struct addsub_block_fixture
{
    addsub_block_fixture()
        : block_container(get_mock_block(ADDSUB_BLOCK, 2, 2))
        , test_addsub(block_container.get_block<addsub_block_control>())
    {
        node_accessor.init_props(test_addsub.get());
    }

    mock_block_container block_container;
    std::shared_ptr<addsub_block_control> test_addsub;
    node_accessor_t node_accessor{};
};

/*
 * This test case ensures that the add/sub block can be added to
 * an RFNoC graph.
 */
BOOST_FIXTURE_TEST_CASE(addsub_test_graph, addsub_block_fixture)
{
    detail::graph_t graph{};

    mock_terminator_t mock_inputs{2, {}, "INPUTS"};
    mock_terminator_t mock_outputs{2, {}, "OUTPUTS"};

    UHD_LOG_INFO("TEST", "Priming mock block properties");
    node_accessor.init_props(&mock_inputs);
    node_accessor.init_props(&mock_outputs);
    mock_outputs.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 0});
    mock_outputs.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 1});

    using graph_edge_t = detail::graph_t::graph_edge_t;

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(
        &mock_inputs, test_addsub.get(), graph_edge_t{0, 0, graph_edge_t::DYNAMIC, true});
    graph.connect(
        &mock_inputs, test_addsub.get(), graph_edge_t{1, 1, graph_edge_t::DYNAMIC, true});
    graph.connect(test_addsub.get(),
        &mock_outputs,
        graph_edge_t{0, 0, graph_edge_t::DYNAMIC, true});
    graph.connect(test_addsub.get(),
        &mock_outputs,
        graph_edge_t{1, 1, graph_edge_t::DYNAMIC, true});

    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");

    // Sanity check the type edge properties on the input node.
    BOOST_CHECK_EQUAL(mock_inputs.get_edge_property<std::string>(
                          "type", {res_source_info::OUTPUT_EDGE, 0}),
        "sc16");
    BOOST_CHECK_EQUAL(mock_inputs.get_edge_property<std::string>(
                          "type", {res_source_info::OUTPUT_EDGE, 1}),
        "sc16");
}

//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/logpwr_block_control.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/register_iface_holder.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

constexpr size_t NUM_CHANS = 4;

/*
 * logpwr_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container, logpwr_blok_control
 * object and node accessor, all of which are accessible to the test case.
 * The instance of the object is destroyed at the end of each test case.
 */
struct logpwr_block_fixture
{
    logpwr_block_fixture()
        : block_container(
            get_mock_block(LOGPWR_BLOCK, NUM_CHANS, NUM_CHANS, uhd::device_addr_t()))
        , test_logpwr(block_container.get_block<logpwr_block_control>())
    {
        node_accessor.init_props(test_logpwr.get());
    }

    mock_block_container block_container;
    std::shared_ptr<logpwr_block_control> test_logpwr;
    node_accessor_t node_accessor{};
};

/*
 * The block controller doesn't do much but constrain its edge type properties
 * to what the block expects (sc16 in; s32 out). This test ensures that a
 * log power block in a graph will propagate its expected types to the
 * source and sink connected to it.
 */
BOOST_FIXTURE_TEST_CASE(logpwr_test_edge_types, logpwr_block_fixture)
{
    detail::graph_t graph{};

    mock_terminator_t mock_source_term(NUM_CHANS);
    mock_terminator_t mock_sink_term(NUM_CHANS);

    UHD_LOG_INFO("TEST", "Creating graph...");
    for (size_t chan = 0; chan < NUM_CHANS; chan++) {
        detail::graph_t::graph_edge_t edge_info{
            chan, chan, detail::graph_t::graph_edge_t::DYNAMIC, true};
        graph.connect(&mock_source_term, test_logpwr.get(), edge_info);
        graph.connect(test_logpwr.get(), &mock_sink_term, edge_info);
    }
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");

    // Now check the type edge properties on the source and sink and ensure
    // they reflect what the log power block needs
    for (size_t chan = 0; chan < NUM_CHANS; chan++) {
        BOOST_CHECK(mock_source_term.get_edge_property<std::string>(
                        "type", {res_source_info::OUTPUT_EDGE, chan})
                    == IO_TYPE_SC16);
        BOOST_CHECK(mock_sink_term.get_edge_property<std::string>(
                        "type", {res_source_info::INPUT_EDGE, chan})
                    == IO_TYPE_S16);
    }
}

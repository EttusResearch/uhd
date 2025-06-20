//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/detail/graph.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/mock_nodes.hpp>
#include <uhd/rfnoc/node_accessor.hpp>
#include <uhd/rfnoc/register_iface_holder.hpp>
#include <rfnoc/gain/gain_block_control.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <memory>

using namespace uhd::rfnoc;
using namespace rfnoc::gain;
using namespace uhd::rfnoc::test;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

constexpr size_t NUM_CHANS = 1;

/*
 * gain_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container, gain_block_control
 * object and node accessor, all of which are accessible to the test case.
 * The instance of the object is destroyed at the end of each test case.
 */
struct gain_block_fixture
{
    gain_block_fixture()
        : block_container(
            get_mock_block(0xb16, NUM_CHANS, NUM_CHANS, uhd::device_addr_t()))
        , test_gain(block_container.get_block<gain_block_control>())
    {
        node_accessor.init_props(test_gain.get());
    }

    mock_block_container block_container;
    std::shared_ptr<gain_block_control> test_gain;
    // The node_accessor is a C++ construct to bypass the public/private
    // division of the underlying C++ class. This should never be used in
    // production outside of unit tests, but here, it lets us peek inside the
    // class to verify it's working as expected.
    node_accessor_t node_accessor{};
};

/*
 * The block controller doesn't do much but constrain its edge type properties
 * to what the block expects (sc16 in; s32 out). This test ensures that a
 * log power block in a graph will propagate its expected types to the
 * source and sink connected to it.
 */
BOOST_FIXTURE_TEST_CASE(gain_test_edge_types, gain_block_fixture)
{
    detail::graph_t graph{};

    mock_terminator_t mock_source_term(NUM_CHANS);
    mock_terminator_t mock_sink_term(NUM_CHANS);

    constexpr size_t chan = 0;

    UHD_LOG_INFO("TEST", "Creating graph...");
    detail::graph_t::graph_edge_t edge_info{
        chan, chan, detail::graph_t::graph_edge_t::DYNAMIC, true};
    graph.connect(&mock_source_term, test_gain.get(), edge_info);
    graph.connect(test_gain.get(), &mock_sink_term, edge_info);

    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");

    // Check if we can use the C++ API to set the gain, then read it back via
    // the property interface
    test_gain->set_gain_value(10);
    BOOST_CHECK_EQUAL(
        test_gain->get_property<int>(gain_block_control::PROP_KEY_GAIN), 10);
    // Now flip it: Write the property, and read back from the C++ API
    test_gain->set_property<int>(gain_block_control::PROP_KEY_GAIN, 20);
    BOOST_CHECK_EQUAL(test_gain->get_gain_value(), 20);
}

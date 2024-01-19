//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/switchboard_block_control.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

/*
 * This class extends mock_reg_iface_t by adding poke and peek hooks that
 * monitor writes and reads to the registers implemented within the
 * Switchboard RFNoC block hardware and emulating the expected behavior of
 * the hardware when those registers are read and written.
 */
const size_t NUM_INPUTS      = 4;
const size_t NUM_OUTPUTS     = 4;
constexpr size_t DEFAULT_MTU = 8000;

class switchboard_mock_reg_iface_t : public mock_reg_iface_t
{
public:
    switchboard_mock_reg_iface_t()
    {
        for (size_t in = 0; in < NUM_INPUTS; in++)
            output_select.push_back(0);
        for (size_t out = 0; out < NUM_OUTPUTS; out++)
            input_select.push_back(0);
    }

    void _poke_cb(
        uint32_t addr, uint32_t data, uhd::time_spec_t /*time*/, bool /*ack*/) override
    {
        size_t chan   = addr / switchboard_block_control::REG_BLOCK_SIZE;
        size_t offset = addr % switchboard_block_control::REG_BLOCK_SIZE;
        if (offset == switchboard_block_control::REG_DEMUX_SELECT_ADDR) {
            output_select[chan] = data;
        } else if (offset == switchboard_block_control::REG_MUX_SELECT_ADDR) {
            input_select[chan] = data;
        } else {
            throw uhd::assertion_error("Invalid write from out of bounds address");
        }
    }

    void _peek_cb(uint32_t addr, uhd::time_spec_t /*time*/) override
    {
        size_t chan   = addr / switchboard_block_control::REG_BLOCK_SIZE;
        size_t offset = addr % switchboard_block_control::REG_BLOCK_SIZE;
        if (offset == switchboard_block_control::REG_DEMUX_SELECT_ADDR) {
            read_memory[addr] = output_select.at(chan);
        } else if (offset == switchboard_block_control::REG_MUX_SELECT_ADDR) {
            read_memory[addr] = input_select.at(chan);
        } else {
            throw uhd::assertion_error("Invalid read from out of bounds address");
        }
    }

    std::vector<uint32_t> input_select{};
    std::vector<uint32_t> output_select{};
};

/* switchboard_block_fixture is a class which is instantiated before each test
 * case is run. It sets up the block container, mock register interface,
 * and mux_block_control object, all of which are accessible to the test
 * case. The instance of the object is destroyed at the end of each test
 * case.
 */

struct switchboard_block_fixture
{
    switchboard_block_fixture()
        : reg_iface(std::make_shared<switchboard_mock_reg_iface_t>())
        , block_container(get_mock_block(SWITCHBOARD_BLOCK,
              NUM_INPUTS,
              NUM_OUTPUTS,
              uhd::device_addr_t(),
              DEFAULT_MTU,
              ANY_DEVICE,
              reg_iface))
        , test_switchboard(block_container.get_block<switchboard_block_control>())
    {
        node_accessor.init_props(test_switchboard.get());
    }

    std::shared_ptr<switchboard_mock_reg_iface_t> reg_iface;
    mock_block_container block_container;
    std::shared_ptr<switchboard_block_control> test_switchboard;
    node_accessor_t node_accessor{};
};

BOOST_FIXTURE_TEST_CASE(swboard_test_construction, switchboard_block_fixture)
{
    // Check that default register values correctly initialized
    for (size_t i = 0; i < NUM_INPUTS; i++)
        BOOST_CHECK_EQUAL(reg_iface->output_select.at(i), 0);
    for (size_t i = 0; i < NUM_OUTPUTS; i++)
        BOOST_CHECK_EQUAL(reg_iface->input_select.at(i), 0);
}

BOOST_FIXTURE_TEST_CASE(swboard_test_connect, switchboard_block_fixture)
{
    // Check that connect() correctly sets register values
    for (size_t i = 0; i < NUM_INPUTS; i++) {
        for (size_t o = 0; o < NUM_OUTPUTS; o++) {
            test_switchboard->connect(i, o);
            BOOST_CHECK_EQUAL(reg_iface->output_select.at(i), o);
            BOOST_CHECK_EQUAL(reg_iface->input_select.at(o), i);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(swboard_test_exceptions, switchboard_block_fixture)
{
    // Check that out of bounds value is not written and exception thrown
    BOOST_CHECK_THROW(test_switchboard->connect(0, NUM_OUTPUTS), std::exception);
    BOOST_CHECK_THROW(test_switchboard->connect(NUM_INPUTS, 0), std::exception);
    BOOST_CHECK_EQUAL(reg_iface->output_select.at(0), 0);
    BOOST_CHECK_EQUAL(reg_iface->input_select.at(0), 0);
}

BOOST_FIXTURE_TEST_CASE(swboard_test_graph, switchboard_block_fixture)
{
    detail::graph_t graph{};

    mock_edge_node_t source{0, NUM_INPUTS, "MOCK_SOURCE"};
    mock_edge_node_t sink{NUM_OUTPUTS, 0, "MOCK_SINK"};

    UHD_LOG_INFO("TEST", "Creating graph...");
    for (size_t port = 0; port < NUM_INPUTS; port++) {
        graph.connect(&source,
            test_switchboard.get(),
            graph_edge_t{port, port, detail::graph_t::graph_edge_t::DYNAMIC, true});
    }
    for (size_t port = 0; port < NUM_OUTPUTS; port++) {
        graph.connect(test_switchboard.get(),
            &sink,
            graph_edge_t{port, port, detail::graph_t::graph_edge_t::DYNAMIC, true});
    }
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
}

//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/keep_one_in_n_block_control.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

/*
 * This class extends mock_reg_iface_t, adding a register peek override
 * that monitors when the WIDTH_N register is peek'd
 */
class keep_one_in_n_mock_reg_iface_t : public mock_reg_iface_t
{
public:
    keep_one_in_n_mock_reg_iface_t(uint32_t width_n) : _width_n(width_n) {}

    void _peek_cb(uint32_t addr, uhd::time_spec_t /*time*/) override
    {
        // Emulate the read-only behavior of the WIDTH_N register
        if (addr == keep_one_in_n_block_control::REG_WIDTH_N_OFFSET) {
            read_memory[addr] = _width_n;
        } else if (addr != keep_one_in_n_block_control::REG_N_OFFSET
                   || addr != keep_one_in_n_block_control::REG_MODE_OFFSET) {
            throw uhd::assertion_error("Invalid read from out of bounds address");
        }
    }

    uint32_t _width_n = 0;
};

/* keep_one_in_n_block_fixture is a class which is instantiated before each test case
 * is run. It sets up the block container, mock register interface, and
 * keep_one_in_n_block_control object, all of which are accessible to the test case.
 * The instance of the object is destroyed at the end of each test case.
 */

namespace {
constexpr size_t WIDTH_N     = 24;
constexpr size_t MAX_N       = (2 << WIDTH_N) - 1;
constexpr size_t DEFAULT_MTU = 8000;
}; // namespace

struct keep_one_in_n_block_fixture
{
    //! Create an Keep One in N block and all related infrastructure for unit testing.
    keep_one_in_n_block_fixture()
        : reg_iface(std::make_shared<keep_one_in_n_mock_reg_iface_t>(WIDTH_N))
        , block_container(get_mock_block(KEEP_ONE_IN_N_BLOCK,
              1,
              1,
              uhd::device_addr_t(),
              DEFAULT_MTU,
              ANY_DEVICE,
              reg_iface))
        , test_keep_one_in_n(block_container.get_block<keep_one_in_n_block_control>())
    {
        node_accessor.init_props(test_keep_one_in_n.get());
    }

    std::shared_ptr<keep_one_in_n_mock_reg_iface_t> reg_iface;
    mock_block_container block_container;
    std::shared_ptr<keep_one_in_n_block_control> test_keep_one_in_n;
    node_accessor_t node_accessor{};
};

/*
 * This test case exercises the Keep One in N block API with valid,
 * in-range values and ensures that the appropriate register is programmed
 * appropriately.
 */
BOOST_FIXTURE_TEST_CASE(keep_one_in_n_test_api, keep_one_in_n_block_fixture)
{
    BOOST_CHECK(test_keep_one_in_n->get_max_n() == MAX_N);

    constexpr int n = 2;
    test_keep_one_in_n->set_n(n);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[keep_one_in_n_block_control::REG_N_OFFSET], n);
    BOOST_CHECK(test_keep_one_in_n->get_n() == n);

    constexpr keep_one_in_n_block_control::mode mode =
        keep_one_in_n_block_control::mode::PACKET_MODE;
    test_keep_one_in_n->set_mode(mode);
    BOOST_CHECK_EQUAL(
        reg_iface->write_memory[keep_one_in_n_block_control::REG_MODE_OFFSET],
        static_cast<uint32_t>(mode));
    BOOST_CHECK(test_keep_one_in_n->get_mode() == mode);
}

/*
 * This test case exercises the range checking performed on several of the
 * Keep One in N block properties, ensuring that the appropriate exception is thrown.
 */
BOOST_FIXTURE_TEST_CASE(keep_one_in_n_test_range_errors, keep_one_in_n_block_fixture)
{
    BOOST_CHECK_THROW(
        test_keep_one_in_n->set_property<int>("n", MAX_N + 1), uhd::value_error);
    BOOST_CHECK_THROW(test_keep_one_in_n->set_property<int>("n", 0), uhd::value_error);
    BOOST_CHECK_THROW(test_keep_one_in_n->set_property<int>("n", -1), uhd::value_error);
    BOOST_CHECK_THROW(
        test_keep_one_in_n->set_property<int>("mode", -1), uhd::value_error);
}

/*
 * This test case ensures that the Keep One in N block controller can be added
 * to a graph.
 */
BOOST_FIXTURE_TEST_CASE(keep_one_in_n_test_graph, keep_one_in_n_block_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_info{
        0, 0, detail::graph_t::graph_edge_t::DYNAMIC, true};

    mock_radio_node_t mock_radio_block{0};
    mock_ddc_node_t mock_ddc_block{};
    mock_terminator_t mock_sink_term(2, {}, "MOCK_SINK");

    UHD_LOG_INFO("TEST", "Priming mock block properties");
    node_accessor.init_props(&mock_radio_block);
    node_accessor.init_props(&mock_ddc_block);
    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 0});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_radio_block, &mock_ddc_block, edge_info);
    graph.connect(&mock_ddc_block, test_keep_one_in_n.get(), edge_info);
    graph.connect(test_keep_one_in_n.get(), &mock_sink_term, edge_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
}

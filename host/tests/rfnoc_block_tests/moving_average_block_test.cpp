//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/moving_average_block_control.hpp>
#include <uhd/rfnoc/null_block_control.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <uhdlib/rfnoc/node_accessor.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/test/unit_test.hpp>
#include <iostream>

using namespace uhd::rfnoc;

// Redeclare this here, since it's only defined outside of UHD_API
noc_block_base::make_args_t::~make_args_t() = default;

BOOST_AUTO_TEST_CASE(test_moving_average_block)
{
    node_accessor_t node_accessor{};
    constexpr size_t num_chans = 1;
    constexpr noc_id_t noc_id  = MOVING_AVERAGE_BLOCK;

    auto block_container           = get_mock_block(noc_id, num_chans, num_chans);
    auto& moving_average_reg_iface = block_container.reg_iface;

    auto test_moving_average = block_container.get_block<moving_average_block_control>();
    BOOST_REQUIRE(test_moving_average);

    node_accessor.init_props(test_moving_average.get());
    UHD_LOG_DEBUG("TEST", "Init done.");

    // Ensure the hardware was programmed correctly with defaults when the
    // block was constructed.
    BOOST_CHECK_EQUAL(moving_average_reg_iface->write_memory.at(
                          moving_average_block_control::REG_SUM_LEN_ADDR),
        10);
    BOOST_CHECK_EQUAL(moving_average_reg_iface->write_memory.at(
                          moving_average_block_control::REG_DIVISOR_ADDR),
        10);

    // Now plop it in a graph
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_info;
    edge_info.src_port        = 0;
    edge_info.dst_port        = 0;
    edge_info.is_forward_edge = true;
    edge_info.edge            = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_terminator_t mock_source_term(1);
    mock_terminator_t mock_sink_term(1);

    // Set up the intial edge properties for the source.
    constexpr double sample_rate = 1e6;
    constexpr size_t mtu         = 4000;
    UHD_LOG_INFO("TEST", "Priming mock source node props");
    mock_source_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<double>(
        "scaling", 1.0, {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<double>(
        "samp_rate", sample_rate, {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<size_t>(
        "mtu", mtu, {res_source_info::OUTPUT_EDGE, 0});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_source_term, test_moving_average.get(), edge_info);
    graph.connect(test_moving_average.get(), &mock_sink_term, edge_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");

    // Change the sum length via the API, and ensure that the hardware is
    // programmed arppropriately with the expected value.
    UHD_LOG_INFO("TEST", "Testing sum length API...");
    BOOST_REQUIRE_THROW(test_moving_average->set_sum_len(0), uhd::value_error);
    constexpr uint8_t new_sum_len = 255;
    test_moving_average->set_sum_len(new_sum_len);
    BOOST_CHECK_EQUAL(moving_average_reg_iface->write_memory.at(
                          moving_average_block_control::REG_SUM_LEN_ADDR),
        new_sum_len);
    BOOST_CHECK_EQUAL(test_moving_average->get_sum_len(), new_sum_len);

    // Change the divisor via the API, and ensure that the hardware is
    // programmed arppropriately with the expected value.
    UHD_LOG_INFO("TEST", "Testing divisor API...");
    BOOST_REQUIRE_THROW(test_moving_average->set_divisor(0), uhd::value_error);
    BOOST_REQUIRE_THROW(test_moving_average->set_divisor(1 << 24), uhd::value_error);
    constexpr uint32_t new_divisor = (1 << 24) - 1;
    test_moving_average->set_divisor(new_divisor);
    BOOST_CHECK_EQUAL(moving_average_reg_iface->write_memory.at(
                          moving_average_block_control::REG_DIVISOR_ADDR),
        new_divisor);
    BOOST_CHECK_EQUAL(test_moving_average->get_divisor(), new_divisor);
}

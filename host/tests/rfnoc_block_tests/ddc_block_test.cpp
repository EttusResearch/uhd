//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/ddc_block_control.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/null_block_control.hpp>
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

BOOST_AUTO_TEST_CASE(test_ddc_block)
{
    node_accessor_t node_accessor{};
    constexpr uint32_t num_hb  = 2;
    constexpr uint32_t max_cic = 128;
    constexpr size_t num_chans = 4;
    constexpr noc_id_t noc_id  = DDC_BLOCK;
    constexpr int TEST_DECIM   = 20;
    constexpr double DEFAULT_RATE = 200e6; // Matches typical MCR of X310

    auto block_container =
        get_mock_block(noc_id, num_chans, num_chans, uhd::device_addr_t("foo=bar"));
    auto& ddc_reg_iface = block_container.reg_iface;
    ddc_reg_iface->read_memory[ddc_block_control::RB_COMPAT_NUM] =
        (ddc_block_control::MAJOR_COMPAT << 16) | ddc_block_control::MINOR_COMPAT;
    ddc_reg_iface->read_memory[ddc_block_control::RB_NUM_HB]        = num_hb;
    ddc_reg_iface->read_memory[ddc_block_control::RB_CIC_MAX_DECIM] = max_cic;
    auto test_ddc = block_container.get_block<ddc_block_control>();
    BOOST_REQUIRE(test_ddc);
    BOOST_CHECK_EQUAL(test_ddc->get_block_args().get("foo"), "bar");

    node_accessor.init_props(test_ddc.get());
    UHD_LOG_DEBUG("TEST", "Init done.");
    test_ddc->set_property<int>("decim", TEST_DECIM, 0);

    BOOST_REQUIRE(ddc_reg_iface->write_memory.count(ddc_block_control::SR_DECIM_ADDR));
    BOOST_CHECK_EQUAL(
        ddc_reg_iface->write_memory.at(ddc_block_control::SR_DECIM_ADDR), 2 << 8 | 5);
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::INPUT_EDGE, 0}), DEFAULT_MTU);

    // Now plop it in a graph
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_info;
    edge_info.src_port        = 0;
    edge_info.dst_port        = 0;
    edge_info.is_forward_edge = true;
    edge_info.edge            = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_terminator_t mock_source_term(1);
    mock_terminator_t mock_sink_term(1);

    UHD_LOG_INFO("TEST", "Priming mock source node props");
    mock_source_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<double>(
        "scaling", 1.0, {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<double>(
        "samp_rate", DEFAULT_RATE, {res_source_info::OUTPUT_EDGE, 0});
    constexpr size_t NEW_MTU = 4000;
    mock_source_term.set_edge_property<size_t>(
        "mtu", NEW_MTU, {res_source_info::OUTPUT_EDGE, 0});

#define CHECK_INPUT_RATE(req_rate)                                           \
    BOOST_REQUIRE_CLOSE(mock_source_term.get_edge_property<double>(          \
                            "samp_rate", {res_source_info::OUTPUT_EDGE, 0}), \
        req_rate,                                                            \
        1e-6);

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_source_term, test_ddc.get(), edge_info);
    graph.connect(test_ddc.get(), &mock_sink_term, edge_info);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
    CHECK_INPUT_RATE(DEFAULT_RATE);
    // We need to set the decimation again, because the rates will screw it
    // change it w.r.t. to the previous setting
    test_ddc->set_property<int>("decim", TEST_DECIM, 0);
    BOOST_CHECK_EQUAL(test_ddc->get_property<int>("decim", 0), TEST_DECIM);
    BOOST_CHECK(mock_source_term.get_edge_property<double>(
                    "samp_rate", {res_source_info::OUTPUT_EDGE, 0})
                == mock_sink_term.get_edge_property<double>(
                       "samp_rate", {res_source_info::INPUT_EDGE, 0})
                       * TEST_DECIM);
    // Input rate should remain unchanged
    CHECK_INPUT_RATE(DEFAULT_RATE);
    BOOST_CHECK(mock_sink_term.get_edge_property<double>(
                    "scaling", {res_source_info::INPUT_EDGE, 0})
                != 1.0);

    BOOST_CHECK_CLOSE(test_ddc->get_frequency_range(0).start(), -DEFAULT_RATE / 2, 1e-6);
    BOOST_CHECK_CLOSE(test_ddc->get_frequency_range(0).stop(), DEFAULT_RATE / 2, 1e-6);
    UHD_LOG_INFO("TEST",
        "Setting freq to 1/8 of input rate (to " << (DEFAULT_RATE / 8) / 1e6 << " MHz)");
    constexpr double TEST_FREQ = DEFAULT_RATE / 8;
    test_ddc->set_property<double>("freq", TEST_FREQ, 0);
    const uint32_t freq_word_1 =
        ddc_reg_iface->write_memory.at(ddc_block_control::SR_FREQ_ADDR);
    BOOST_REQUIRE(freq_word_1 != 0);
    UHD_LOG_INFO(
        "TEST", "Doubling input rate (to " << (DEFAULT_RATE / 4) / 1e6 << " MHz)");
    // Now this should change the freq word, but not the absolute frequency
    mock_source_term.set_edge_property<double>(
        "samp_rate", DEFAULT_RATE * 2, {res_source_info::OUTPUT_EDGE, 0});
    const double freq_word_2 =
        ddc_reg_iface->write_memory.at(ddc_block_control::SR_FREQ_ADDR);
    // The frequency word is the phase increment, which will halve. We skirt
    // around fixpoint/floating point accuracy issues by using CLOSE.
    BOOST_CHECK_CLOSE(double(freq_word_1) / double(freq_word_2), 2.0, 1e-6);

    UHD_LOG_INFO("TEST", "Testing DDC MTU propagation");
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::INPUT_EDGE, 0}), NEW_MTU);
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::OUTPUT_EDGE, 0}), NEW_MTU);
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::INPUT_EDGE, 1}), DEFAULT_MTU);
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::OUTPUT_EDGE, 1}), DEFAULT_MTU);
    mock_source_term.set_edge_property<size_t>(
        "mtu", NEW_MTU / 2, {res_source_info::OUTPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::INPUT_EDGE, 0}), NEW_MTU / 2);
    BOOST_CHECK_EQUAL(test_ddc->get_mtu({res_source_info::OUTPUT_EDGE, 0}), NEW_MTU / 2);

    // Now reset the props using set_properties
    test_ddc->set_properties(uhd::device_addr_t("decim=1,freq=0.0,foo=bar"), 0);
    BOOST_CHECK_EQUAL(test_ddc->get_property<int>("decim", 0), 1);
    BOOST_CHECK_EQUAL(test_ddc->get_property<double>("freq", 0), 0.0);
}

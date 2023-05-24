//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include "ferrum_radio_mock.hpp"
#include "x4xx_fbx_mpm_mock.hpp"
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/rfnoc/graph.hpp>
#include <boost/test/unit_test.hpp>
#include <cstddef>
#include <iostream>
#include <thread>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace std::chrono_literals;
using namespace uhd::usrp::fbx;
using namespace uhd::experts;

BOOST_FIXTURE_TEST_CASE(fbx_api_freq_tx_test, x400_radio_fixture)
{
    const std::string log = "FBX_API_TX_FREQUENCY_TEST";
    const double ep       = 10;
    // TODO: consult step size
    uhd::freq_range_t fbx_freq(FBX_MIN_FREQ, FBX_MAX_FREQ, 100e6);
    for (const size_t chan : {0, 1, 2, 3}) {
        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " FREQ CHANGE (SET->RETURN)\n");
        for (double iter = fbx_freq.start(); iter <= fbx_freq.stop();
             iter += fbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            const double freq = test_radio->set_tx_frequency(iter, chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }

        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " FREQ CHANGE (SET->GET)\n");
        for (double iter = fbx_freq.start(); iter <= fbx_freq.stop();
             iter += fbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            test_radio->set_tx_frequency(iter, chan);
            const double freq = test_radio->get_tx_frequency(chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(fbx_api_freq_rx_test, x400_radio_fixture)
{
    const std::string log = "FBX_API_RX_FREQUENCY_TEST";
    const double ep       = 10;
    // TODO: consult step size
    uhd::freq_range_t fbx_freq(FBX_MIN_FREQ, FBX_MAX_FREQ, 100e6);

    for (const size_t chan : {0, 1, 2, 3}) {
        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " FREQ CHANGE (SET->RETURN)\n");
        for (double iter = fbx_freq.start(); iter <= fbx_freq.stop();
             iter += fbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            const double freq = test_radio->set_rx_frequency(iter, chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }
        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " FREQ CHANGE (SET->GET\n");
        for (double iter = fbx_freq.start(); iter <= fbx_freq.stop();
             iter += fbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            test_radio->set_rx_frequency(iter, chan);
            const double freq = test_radio->get_rx_frequency(chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(fbx_frequency_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "FBX_FREQUENCY_TEST";
    const double ep       = 10;
    // TODO: consult step size
    uhd::freq_range_t fbx_freq(FBX_MIN_FREQ, FBX_MAX_FREQ, 100e6);

    for (const auto& fe_path : {
             fs_path("dboard/tx_frontends/0"),
             fs_path("dboard/tx_frontends/1"),
             fs_path("dboard/rx_frontends/0"),
             fs_path("dboard/rx_frontends/1"),
         }) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " FREQ CHANGE\n");
        for (double iter = fbx_freq.start(); iter <= fbx_freq.stop();
             iter += fbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            tree->access<double>(fe_path / "freq").set(iter);

            const double ret_value = tree->access<double>(fe_path / "freq").get();

            BOOST_REQUIRE(abs(iter - ret_value) < ep);
        }
    }
}
/*****************************************************************************************
 * In X440, all channels per DB share a sync switch for internal or external sync. So when
 * setting e.g. ch0 to sync_ext and then setting ch1 to sync_int, this influences ch0,
 * too. At the same time this shouldn't have any influence on other channels' antennas.
 * This will be tested here. BOOST_FIXTURE_TEST_CASE(fbx_sync_antenna_test,
 * x400_radio_fixture)
 *****************************************************************************************/
BOOST_FIXTURE_TEST_CASE(fbx_rx_antenna_test, x400_radio_fixture)
{
    // First check that setting and getting the antennas works
    for (const std::string& ant : test_radio->get_rx_antennas(0)) {
        test_radio->set_rx_antenna(ant, 0);
        BOOST_REQUIRE_EQUAL(ant, test_radio->get_rx_antenna(0));
    }

    // Then check the internal and external sync limitation. First reset everything to RX
    // port
    for (size_t channel = 0; channel < FBX_MAX_NUM_CHANS; channel++) {
        test_radio->set_rx_antenna(ANTENNA_RX, channel);
    }

    // Then set a single channel to SYNC_INT and check the others, they must remain at RX
    test_radio->set_rx_antenna(ANTENNA_SYNC_INT, 0);
    BOOST_REQUIRE_EQUAL(ANTENNA_SYNC_INT, test_radio->get_rx_antenna(0));
    for (size_t channel = 1; channel < FBX_MAX_NUM_CHANS; channel++) {
        BOOST_REQUIRE_EQUAL(ANTENNA_RX, test_radio->get_rx_antenna(channel));
    }

    // Now we change ch1 to SYNC_EXT. This should change ch0, too, but not the others.
    test_radio->set_rx_antenna(ANTENNA_SYNC_EXT, 1);
    BOOST_REQUIRE_EQUAL(ANTENNA_SYNC_EXT, test_radio->get_rx_antenna(1));
    BOOST_REQUIRE_EQUAL(ANTENNA_SYNC_EXT, test_radio->get_rx_antenna(0));
    BOOST_REQUIRE_EQUAL(ANTENNA_RX, test_radio->get_rx_antenna(2));
    BOOST_REQUIRE_EQUAL(ANTENNA_RX, test_radio->get_rx_antenna(3));
}

BOOST_FIXTURE_TEST_CASE(fbx_tx_antenna_test, x400_radio_fixture)
{
    auto prev_ant = test_radio->get_tx_antenna(0);
    for (const std::string& ant : test_radio->get_tx_antennas(0)) {
        test_radio->set_tx_antenna(ant, 0);
        BOOST_REQUIRE_EQUAL(ant, test_radio->get_tx_antenna(0));
        // Check that setting a different channel won't change the original channel
        test_radio->set_tx_antenna(prev_ant, 1);
        BOOST_REQUIRE_EQUAL(ant, test_radio->get_tx_antenna(0));
        prev_ant = ant;
    }
}

/******************************************************************************
 * RFNoC Graph Test
 *
 * This test case ensures that the Radio Block can be added to an RFNoC graph.
 *****************************************************************************/
BOOST_FIXTURE_TEST_CASE(x400_radio_test_graph, x400_radio_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_port_info0;
    edge_port_info0.src_port = 0;
    edge_port_info0.dst_port = 0;
    // edge_port_info0.property_propagation_active = true;
    edge_port_info0.edge = detail::graph_t::graph_edge_t::DYNAMIC;
    detail::graph_t::graph_edge_t edge_port_info1;
    edge_port_info1.src_port = 1;
    edge_port_info1.dst_port = 1;
    // edge_port_info1.property_propagation_active = true;
    edge_port_info1.edge = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_radio_node_t mock_radio_block{0};
    mock_terminator_t mock_sink_term(2, {}, "MOCK_SINK");
    mock_terminator_t mock_source_term(2, {}, "MOCK_SOURCE");

    UHD_LOG_INFO("TEST", "Priming mock block properties");
    node_accessor.init_props(&mock_radio_block);
    mock_source_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::OUTPUT_EDGE, 1});
    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 0});
    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 1});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_source_term, test_radio.get(), edge_port_info0);
    graph.connect(&mock_source_term, test_radio.get(), edge_port_info1);
    graph.connect(test_radio.get(), &mock_sink_term, edge_port_info0);
    graph.connect(test_radio.get(), &mock_sink_term, edge_port_info1);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
}

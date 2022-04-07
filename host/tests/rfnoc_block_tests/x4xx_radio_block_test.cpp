//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "../rfnoc_graph_mock_nodes.hpp"
#include "x4xx_radio_mock.hpp"
#include "x4xx_zbx_mpm_mock.hpp"
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
using namespace uhd::usrp::zbx;
using namespace uhd::experts;

/******************************************************************************
 * RFNoC Graph Test
 *
 * This test case ensures that the Radio Block can be added to an RFNoC graph.
 *****************************************************************************/
BOOST_FIXTURE_TEST_CASE(x400_radio_test_graph, x400_radio_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_port_info0;
    edge_port_info0.src_port        = 0;
    edge_port_info0.dst_port        = 0;
    edge_port_info0.is_forward_edge = true;
    edge_port_info0.edge            = detail::graph_t::graph_edge_t::DYNAMIC;
    detail::graph_t::graph_edge_t edge_port_info1;
    edge_port_info1.src_port        = 1;
    edge_port_info1.dst_port        = 1;
    edge_port_info1.is_forward_edge = true;
    edge_port_info1.edge            = detail::graph_t::graph_edge_t::DYNAMIC;

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

/******************************************************************************
 * RFNoC atomic item size property test
 *
 * This test case ensures that the radio block propagates atomic item size correctly
 *****************************************************************************/
BOOST_FIXTURE_TEST_CASE(x400_radio_test_prop_prop, x400_radio_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_port_info0;
    edge_port_info0.src_port        = 0;
    edge_port_info0.dst_port        = 0;
    edge_port_info0.is_forward_edge = true;
    edge_port_info0.edge            = detail::graph_t::graph_edge_t::DYNAMIC;
    detail::graph_t::graph_edge_t edge_port_info1;
    edge_port_info1.src_port        = 1;
    edge_port_info1.dst_port        = 1;
    edge_port_info1.is_forward_edge = true;
    edge_port_info1.edge            = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_terminator_t mock_sink_term(2, {}, "MOCK_SINK");
    mock_terminator_t mock_source_term(2, {}, "MOCK_SOURCE");

    UHD_LOG_INFO("TEST", "Priming mock block properties");
    mock_source_term.set_edge_property<size_t>(
        "atomic_item_size", 1, {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<size_t>(
        "atomic_item_size", 1, {res_source_info::OUTPUT_EDGE, 1});
    mock_source_term.set_edge_property<size_t>(
        "mtu", 99, {res_source_info::OUTPUT_EDGE, 0});
    mock_sink_term.set_edge_property<size_t>(
        "atomic_item_size", 999, {res_source_info::INPUT_EDGE, 0});
    mock_sink_term.set_edge_property<size_t>(
        "atomic_item_size", 999, {res_source_info::INPUT_EDGE, 1});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_source_term, test_radio.get(), edge_port_info0);
    graph.connect(&mock_source_term, test_radio.get(), edge_port_info1);
    graph.connect(test_radio.get(), &mock_sink_term, edge_port_info0);
    graph.connect(test_radio.get(), &mock_sink_term, edge_port_info1);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();

    UHD_LOG_INFO("TEST", "Testing atomic item size propagation...");

    // radio has a sample width of 32 bits (sc16) and spc of 1 by default
    // this results in a atomic item size of 4 for the radio
    // because property gets propagated immediately after setting in
    // the committed graph we can check the result of the propagation
    // at the mock edges

    mock_source_term.set_edge_property<size_t>(
        "atomic_item_size", 1, {res_source_info::OUTPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(mock_source_term.get_edge_property<size_t>(
                          "atomic_item_size", {res_source_info::OUTPUT_EDGE, 0}),
        4);

    mock_source_term.set_edge_property<size_t>(
        "atomic_item_size", 4, {res_source_info::OUTPUT_EDGE, 1});
    BOOST_CHECK_EQUAL(mock_source_term.get_edge_property<size_t>(
                          "atomic_item_size", {res_source_info::OUTPUT_EDGE, 1}),
        4);

    mock_source_term.set_edge_property<size_t>(
        "atomic_item_size", 9, {res_source_info::OUTPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(mock_source_term.get_edge_property<size_t>(
                          "atomic_item_size", {res_source_info::OUTPUT_EDGE, 0}),
        36);

    mock_source_term.set_edge_property<size_t>(
        "atomic_item_size", 10, {res_source_info::OUTPUT_EDGE, 1});
    BOOST_CHECK_EQUAL(mock_source_term.get_edge_property<size_t>(
                          "atomic_item_size", {res_source_info::OUTPUT_EDGE, 1}),
        20);

    mock_source_term.set_edge_property<size_t>(
        "mtu", 99, {res_source_info::OUTPUT_EDGE, 0});
    mock_source_term.set_edge_property<size_t>(
        "atomic_item_size", 25, {res_source_info::OUTPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(mock_source_term.get_edge_property<size_t>(
                          "atomic_item_size", {res_source_info::OUTPUT_EDGE, 0}),
        96);

    // repeat for sink
    mock_sink_term.set_edge_property<size_t>(
        "atomic_item_size", 1, {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(mock_sink_term.get_edge_property<size_t>(
                          "atomic_item_size", {res_source_info::INPUT_EDGE, 0}),
        4);

    mock_sink_term.set_edge_property<size_t>(
        "atomic_item_size", 4, {res_source_info::INPUT_EDGE, 1});
    BOOST_CHECK_EQUAL(mock_sink_term.get_edge_property<size_t>(
                          "atomic_item_size", {res_source_info::INPUT_EDGE, 1}),
        4);

    mock_sink_term.set_edge_property<size_t>(
        "atomic_item_size", 7, {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(mock_sink_term.get_edge_property<size_t>(
                          "atomic_item_size", {res_source_info::INPUT_EDGE, 0}),
        28);
    BOOST_CHECK_EQUAL(test_radio->get_property<int>("spp", 0) % 7, 0);

    mock_sink_term.set_edge_property<size_t>(
        "atomic_item_size", 22, {res_source_info::INPUT_EDGE, 1});
    BOOST_CHECK_EQUAL(mock_sink_term.get_edge_property<size_t>(
                          "atomic_item_size", {res_source_info::INPUT_EDGE, 1}),
        44);
    BOOST_CHECK_EQUAL(test_radio->get_property<int>("spp", 1) % 11, 0);

    mock_sink_term.set_edge_property<size_t>(
        "mtu", 179, {res_source_info::INPUT_EDGE, 0});
    mock_sink_term.set_edge_property<size_t>(
        "atomic_item_size", 46, {res_source_info::INPUT_EDGE, 0});
    BOOST_CHECK_EQUAL(mock_sink_term.get_edge_property<size_t>(
                          "atomic_item_size", {res_source_info::INPUT_EDGE, 0}),
        92);
    BOOST_CHECK_EQUAL(test_radio->get_property<int>("spp", 0), 23);

    test_radio->set_property<int>("spp", 3, 0);
    BOOST_CHECK_EQUAL(test_radio->get_property<int>("spp", 0), 23);
    test_radio->set_property<int>("spp", 24, 0);
    BOOST_CHECK_EQUAL(test_radio->get_property<int>("spp", 0), 23);

    // If we go higher, then there's no valid spp value that is both smaller than
    // MTU *and* a multiple of AIS
    UHD_LOG_INFO("TEST", "Expecting ERROR here VVV");
    BOOST_REQUIRE_THROW(mock_sink_term.set_edge_property<size_t>(
                            "atomic_item_size", 47, {res_source_info::INPUT_EDGE, 0}),
        uhd::resolve_error);
    UHD_LOG_INFO("TEST", "Expecting ERROR here ^^^");
}

BOOST_FIXTURE_TEST_CASE(zbx_api_freq_tx_test, x400_radio_fixture)
{
    const std::string log = "ZBX_API_TX_FREQUENCY_TEST";
    const double ep       = 10;
    // TODO: consult step size
    uhd::freq_range_t zbx_freq(ZBX_MIN_FREQ, ZBX_MAX_FREQ, 100e6);
    for (size_t chan : {0, 1}) {
        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " FREQ CHANGE (SET->RETURN)\n");
        for (double iter = zbx_freq.start(); iter <= zbx_freq.stop();
             iter += zbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            const double freq = test_radio->set_tx_frequency(iter, chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }

        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " FREQ CHANGE (SET->GET)\n");
        for (double iter = zbx_freq.start(); iter <= zbx_freq.stop();
             iter += zbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            test_radio->set_tx_frequency(iter, chan);
            const double freq = test_radio->get_tx_frequency(chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_api_freq_rx_test, x400_radio_fixture)
{
    const std::string log = "ZBX_API_RX_FREQUENCY_TEST";
    const double ep       = 10;
    // TODO: consult step size
    uhd::freq_range_t zbx_freq(ZBX_MIN_FREQ, ZBX_MAX_FREQ, 100e6);

    for (size_t chan : {0, 1}) {
        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " FREQ CHANGE (SET->RETURN)\n");
        for (double iter = zbx_freq.start(); iter <= zbx_freq.stop();
             iter += zbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            const double freq = test_radio->set_rx_frequency(iter, chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }
        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " FREQ CHANGE (SET->GET\n");
        for (double iter = zbx_freq.start(); iter <= zbx_freq.stop();
             iter += zbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            test_radio->set_rx_frequency(iter, chan);
            const double freq = test_radio->get_rx_frequency(chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_frequency_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX_FREQUENCY_TEST";
    const double ep       = 10;
    // TODO: consult step size
    uhd::freq_range_t zbx_freq(ZBX_MIN_FREQ, ZBX_MAX_FREQ, 100e6);

    for (auto fe_path : {
             fs_path("dboard/tx_frontends/0"),
             fs_path("dboard/tx_frontends/1"),
             fs_path("dboard/rx_frontends/0"),
             fs_path("dboard/rx_frontends/1"),
         }) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " FREQ CHANGE\n");
        for (double iter = zbx_freq.start(); iter <= zbx_freq.stop();
             iter += zbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            tree->access<double>(fe_path / "freq").set(iter);

            const double ret_value = tree->access<double>(fe_path / "freq").get();

            BOOST_REQUIRE(abs(iter - ret_value) < ep);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_api_tx_gain_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX TX GAIN TEST";
    uhd::freq_range_t zbx_gain(TX_MIN_GAIN, TX_MAX_GAIN, 1);

    for (size_t chan : {0, 1}) {
        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " GAIN CHANGE (SET->RETURN)\n");
        for (double iter = zbx_gain.start(); iter <= zbx_gain.stop();
             iter += zbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);

            const double ret_gain = test_radio->set_tx_gain(iter, chan);

            BOOST_CHECK_EQUAL(iter, ret_gain);
        }
        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " GAIN CHANGE (SET->GET)\n");
        for (double iter = zbx_gain.start(); iter <= zbx_gain.stop();
             iter += zbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);

            test_radio->set_tx_gain(iter, chan);
            const double ret_gain = test_radio->get_tx_gain(chan);

            BOOST_CHECK_EQUAL(iter, ret_gain);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_api_tx_gain_stage_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX API TX GAIN STAGE TEST";

    for (size_t chan : {0, 1}) {
        test_radio->set_tx_gain_profile(ZBX_GAIN_PROFILE_MANUAL, chan);

        UHD_LOG_INFO(
            log, "BEGIN TEST: tx" << chan << " GAIN STAGE CHANGE (SET->RETURN)\n");
        for (auto gain_stage : ZBX_TX_GAIN_STAGES) {
            if (gain_stage == ZBX_GAIN_STAGE_AMP) {
                for (double amp : {ZBX_TX_LOWBAND_GAIN, ZBX_TX_HIGHBAND_GAIN}) {
                    UHD_LOG_INFO(log, "Testing dsa: " << amp);
                    const double ret_gain =
                        test_radio->set_tx_gain(amp, gain_stage, chan);
                    UHD_LOG_INFO(log, "return: " << ret_gain);
                    BOOST_CHECK_EQUAL(amp, ret_gain);
                }
            } else {
                for (unsigned int iter = 0; iter <= ZBX_TX_DSA_MAX_ATT; iter++) {
                    UHD_LOG_INFO(log, "Testing dsa: " << iter);
                    const double ret_gain =
                        test_radio->set_tx_gain(iter, gain_stage, chan);
                    BOOST_CHECK_EQUAL(iter, ret_gain);
                }
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_api_tx_gain_stage_test_set_get, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX API TX GAIN STAGE TEST";

    for (size_t chan : {0, 1}) {
        test_radio->set_tx_gain_profile(ZBX_GAIN_PROFILE_MANUAL, chan);
        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " GAIN STAGE CHANGE (SET->GET)\n");
        for (auto gain_stage : ZBX_TX_GAIN_STAGES) {
            if (gain_stage == ZBX_GAIN_STAGE_AMP) {
                for (double amp :
                    {/*ZBX_TX_BYPASS_GAIN, currently disabled*/ ZBX_TX_LOWBAND_GAIN,
                        ZBX_TX_HIGHBAND_GAIN}) {
                    UHD_LOG_INFO(log, "Testing amp: " << amp);
                    test_radio->set_tx_gain(amp, gain_stage, chan);
                    const double ret_gain = test_radio->get_tx_gain(gain_stage, chan);
                    BOOST_CHECK_EQUAL(amp, ret_gain);
                }
            } else {
                for (unsigned int iter = 0; iter <= ZBX_TX_DSA_MAX_ATT; iter++) {
                    UHD_LOG_INFO(log, "Testing dsa: " << iter);
                    test_radio->set_tx_gain(iter, gain_stage, chan);
                    const double ret_gain = test_radio->get_tx_gain(gain_stage, chan);
                    BOOST_CHECK_EQUAL(iter, ret_gain);
                }
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_api_rx_gain_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX RX API GAIN TEST";
    uhd::freq_range_t zbx_gain(TX_MIN_GAIN, TX_MAX_GAIN, 1);

    for (size_t chan : {0, 1}) {
        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " GAIN CHANGE (SET->RETURN)\n");
        for (double iter = zbx_gain.start(); iter <= zbx_gain.stop();
             iter += zbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);

            const double ret_gain = test_radio->set_rx_gain(iter, chan);

            BOOST_CHECK_EQUAL(iter, ret_gain);
        }
        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " GAIN CHANGE (SET->GET)\n");
        for (double iter = zbx_gain.start(); iter <= zbx_gain.stop();
             iter += zbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);

            test_radio->set_rx_gain(iter, chan);
            const double ret_gain = test_radio->get_rx_gain(chan);

            BOOST_CHECK_EQUAL(iter, ret_gain);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_api_rx_gain_stage_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX API RX GAIN STAGE TEST";

    for (size_t chan : {0, 1}) {
        test_radio->set_rx_gain_profile(ZBX_GAIN_PROFILE_MANUAL, chan);

        UHD_LOG_INFO(
            log, "BEGIN TEST: rx" << chan << " GAIN STAGE CHANGE (SET->RETURN)\n");
        for (auto gain_stage : ZBX_RX_GAIN_STAGES) {
            for (unsigned int iter = 0; iter <= ZBX_RX_DSA_MAX_ATT; iter++) {
                UHD_LOG_INFO(log, "Testing dsa: " << gain_stage << " " << iter);
                const double ret_gain = test_radio->set_rx_gain(iter, gain_stage, chan);

                BOOST_CHECK_EQUAL(iter, ret_gain);
            }
        }

        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " GAIN STAGE CHANGE (SET->GET)\n");
        for (auto gain_stage : ZBX_RX_GAIN_STAGES) {
            for (unsigned int iter = 0; iter <= ZBX_RX_DSA_MAX_ATT; iter++) {
                UHD_LOG_INFO(log, "Testing " << gain_stage << " " << iter);

                test_radio->set_rx_gain(iter, gain_stage, chan);
                const double ret_gain = test_radio->get_rx_gain(gain_stage, chan);

                BOOST_CHECK_EQUAL(iter, ret_gain);
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_tx_gain_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX GAIN TEST";
    uhd::freq_range_t zbx_gain(TX_MIN_GAIN, TX_MAX_GAIN, 1);

    for (auto fe_path :
        {fs_path("dboard/tx_frontends/0"), fs_path("dboard/tx_frontends/1")}) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " GAIN CHANGE\n");
        for (double iter = zbx_gain.start(); iter <= zbx_gain.stop();
             iter += zbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);
            const auto gain_path = fe_path / "gains" / ZBX_GAIN_STAGE_ALL / "value";
            tree->access<double>(gain_path).set(iter);
            const double ret_gain = tree->access<double>(gain_path).get();
            BOOST_CHECK_EQUAL(iter, ret_gain);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_rx_gain_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX GAIN TEST";
    uhd::freq_range_t zbx_gain(RX_MIN_GAIN, RX_MAX_GAIN, 1);

    for (auto fe_path :
        {fs_path("dboard/rx_frontends/0"), fs_path("dboard/rx_frontends/1")}) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " GAIN CHANGE\n");
        for (double iter = zbx_gain.start(); iter <= zbx_gain.stop();
             iter += zbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);
            const auto gain_path = fe_path / "gains" / ZBX_GAIN_STAGE_ALL / "value";
            tree->access<double>(gain_path).set(iter);
            const double ret_gain = tree->access<double>(gain_path).get();
            BOOST_CHECK_EQUAL(iter, ret_gain);
        }
    }
}

// Have to be careful about LO testing; it'll throw off the coerced frequency a bunch,
// possibly to illegal values like negative frequencies, and could make the gain API
// freak out. We use the center frequency to set initial mixer values, then try to test
// all LO's in the valid zbx range.
// TODO: expand this
const std::map<double, std::vector<std::array<double, 2>>> valid_lo_freq_map = {
    {1e9, {{4.5e9, 4.5e9}, {5e9, 5e9}, {5.5e9, 5.5e9}, {6e9, 6e9}}},
    {2e9, {{4.5e9, 4.5e9}, {5e9, 5e9}, {5.5e9, 5.5e9}, {6e9, 6e9}}}};

// TODO: More frequencies_are_equal issues, too much variance
BOOST_FIXTURE_TEST_CASE(zbx_api_tx_lo_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX TX TEST";
    const double ep       = 10;

    for (size_t chan : {0, 1}) {
        UHD_LOG_INFO(log, "BEGIN TEST: TX" << chan << " FREQ CHANGE (SET->RETURN)\n");
        for (auto iter = valid_lo_freq_map.begin(); iter != valid_lo_freq_map.end();
             iter++) {
            for (auto iter_lo = iter->second.begin(); iter_lo != iter->second.end();
                 iter_lo++) {
                // Just so we're clear about our value mapping
                const double req_freq = iter->first;
                const double req_lo1  = iter_lo->at(0);
                const double req_lo2  = iter_lo->at(1);

                UHD_LOG_INFO(log,
                    "Testing center freq " << req_freq / 1e6 << "MHz, lo1 freq "
                                           << req_lo1 / 1e6 << "MHz, lo2 freq "
                                           << req_lo2 / 1e6 << "MHz");
                // Need to set center frequency first, it'll set all the mixer values
                test_radio->set_tx_frequency(iter->first, chan);
                const double lo1_ret =
                    test_radio->set_tx_lo_freq(iter_lo->at(0), ZBX_LO1, chan);
                const double lo2_ret =
                    test_radio->set_tx_lo_freq(iter_lo->at(1), ZBX_LO2, chan);
                // No use comparing set_tx_freq, we've already ran that test and
                // get_tx_frequency would return who knows what at this point
                BOOST_REQUIRE(abs(iter_lo->at(0) - lo1_ret) < ep);
                BOOST_REQUIRE(abs(iter_lo->at(1) - lo2_ret) < ep);
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_api_rx_lo_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX RX LO TEST";
    const double ep       = 10;

    for (size_t chan : {0, 1}) {
        UHD_LOG_INFO(log, "BEGIN TEST: RX" << chan << " FREQ CHANGE (SET->RETURN)\n");
        for (auto iter = valid_lo_freq_map.begin(); iter != valid_lo_freq_map.end();
             iter++) {
            for (auto iter_lo = iter->second.begin(); iter_lo != iter->second.end();
                 iter_lo++) {
                // Just so we're clear about our value mapping
                const double req_freq = iter->first;
                const double req_lo1  = iter_lo->at(0);
                const double req_lo2  = iter_lo->at(1);

                UHD_LOG_INFO(log,
                    "Testing center freq " << req_freq / 1e6 << "MHz, lo1 freq "
                                           << req_lo1 / 1e6 << "MHz, lo2 freq "
                                           << req_lo2 / 1e6 << "MHz");
                // Need to set center frequency first, it'll set all the mixer values
                test_radio->set_rx_frequency(iter->first, chan);
                const double lo1_ret =
                    test_radio->set_rx_lo_freq(iter_lo->at(0), ZBX_LO1, chan);
                const double lo2_ret =
                    test_radio->set_rx_lo_freq(iter_lo->at(1), ZBX_LO2, chan);
                // No use comparing set_tx_freq, we've already ran that test and
                // get_tx_frequency would return who knows what at this point
                BOOST_REQUIRE(abs(iter_lo->at(0) - lo1_ret) < ep);
                BOOST_REQUIRE(abs(iter_lo->at(1) - lo2_ret) < ep);
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_lo_tree_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX LO1 TEST";
    const double ep       = 10;

    for (auto fe_path : {
             fs_path("dboard/tx_frontends/0"),
             fs_path("dboard/tx_frontends/1"),
             fs_path("dboard/rx_frontends/0"),
             fs_path("dboard/rx_frontends/1"),
         }) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " LO FREQ CHANGE (SET->RETURN)\n");
        for (auto iter = valid_lo_freq_map.begin(); iter != valid_lo_freq_map.end();
             iter++) {
            for (auto iter_lo = iter->second.begin(); iter_lo != iter->second.end();
                 iter_lo++) {
                // Just so we're clear about our value mapping
                const double req_freq = iter->first;
                const double req_lo1  = iter_lo->at(0);
                const double req_lo2  = iter_lo->at(1);
                UHD_LOG_INFO(log,
                    "Testing lo1 freq " << req_lo1 / 1e6 << "MHz, lo2 freq "
                                        << req_lo2 / 1e6 << "MHz at center frequency "
                                        << req_freq / 1e6 << "MHz");
                tree->access<double>(fe_path / "freq").set(req_freq);
                const double ret_lo1 =
                    tree->access<double>(fe_path / "los" / ZBX_LO1 / "freq" / "value")
                        .set(req_lo1)
                        .get();
                const double ret_lo2 =
                    tree->access<double>(fe_path / "los" / ZBX_LO2 / "freq" / "value")
                        .set(req_lo2)
                        .get();
                BOOST_REQUIRE(abs(req_lo1 - ret_lo1) < ep);
                BOOST_REQUIRE(abs(req_lo2 - ret_lo2) < ep);
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_custom_tx_tune_table_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    constexpr size_t chan = 0;
    constexpr double ep   = 10.0;

    test_radio->set_tx_frequency(4.4e9, chan);

    BOOST_REQUIRE(abs(test_radio->get_tx_lo_freq(ZBX_LO2, chan) - 5447680000.0) < ep);

    // Custom TX tune table to try. This table is identical to the normal table,
    // except it only has one entry (4.03GHz-4.5GHz) and the IF2 frequency for
    // that band is 10MHz lower (chosen arbitrarily)
    // Turn clang-formatting off so it doesn't compress these tables into a mess.
    // clang-format off
    static const std::vector<tune_map_item_t> alternate_tx_tune_map = {
    //  | min_band_freq | max_band_freq | rf_fir | if1_fir | if2_fir | mix1 m, n | mix2 m, n | if1_freq_min | if1_freq_max | if2_freq_min | if2_freq_max |
        {   4030e6,         4500e6,          0,       1,        1,       0,  0,     -1,  1,            0,             0,        1050e6,        1050e6    },
    };

    // Turn clang-format back on just for posterity
    // clang-format on

    tree->access<std::vector<uhd::usrp::zbx::tune_map_item_t>>(
            "dboard/tx_frontends/0/tune_table")
        .set(alternate_tx_tune_map);

    BOOST_REQUIRE(abs(test_radio->get_tx_lo_freq(ZBX_LO2, chan) - 5437440000.0) < ep);
}

BOOST_FIXTURE_TEST_CASE(zbx_custom_rx_tune_table_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    constexpr size_t chan = 0;
    constexpr double ep   = 10.0;

    test_radio->set_rx_frequency(4.4e9, chan);

    BOOST_REQUIRE(abs(test_radio->get_rx_lo_freq(ZBX_LO2, chan) - 6236160000.0) < ep);

    // Custom RX tune table to try. This table is identical to the normal table,
    // except it only has one entry (4.2GHz-4.5GHz) and the IF2 frequency for
    // that band is 10MHz lower (chosen arbitrarily)
    // Turn clang-formatting off so it doesn't compress these tables into a mess.
    // clang-format off
    static const std::vector<tune_map_item_t> alternate_rx_tune_map = {
    //  | min_band_freq | max_band_freq | rf_fir | if1_fir | if2_fir | mix1 m, n | mix2 m, n | if1_freq_min | if1_freq_max | if2_freq_min | if2_freq_max |
        {   4200e6,         4500e6,          0,       2,        2,       0,  0,     -1,  1,            0,             0,        1840e6,        1840e6    },
    };

    // Turn clang-format back on just for posterity
    // clang-format on

    tree->access<std::vector<uhd::usrp::zbx::tune_map_item_t>>(
            "dboard/rx_frontends/0/tune_table")
        .set(alternate_rx_tune_map);

    BOOST_REQUIRE(abs(test_radio->get_rx_lo_freq(ZBX_LO2, chan) - 6225920000.0) < ep);
}

BOOST_FIXTURE_TEST_CASE(zbx_ant_test, x400_radio_fixture)
{
    auto tree       = test_radio->get_tree();
    std::string log = "ZBX RX ANTENNA TEST";

    for (auto fe_path :
        {fs_path("dboard/rx_frontends/0"), fs_path("dboard/rx_frontends/1")}) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " ANTENNA CHANGE\n");
        for (auto iter : RX_ANTENNAS) {
            UHD_LOG_INFO(log, "Testing Antenna: " << iter);

            tree->access<std::string>(fe_path / "antenna/value").set(iter);

            std::string ret_ant =
                tree->access<std::string>(fe_path / "antenna/value").get();
            BOOST_CHECK_EQUAL(iter, ret_ant);
        }
    }
    for (size_t chan = 0; chan < 2; chan++) {
        for (auto iter : RX_ANTENNAS) {
            UHD_LOG_INFO(log, "Testing Antenna: " << iter);

            test_radio->set_rx_antenna(iter, chan);

            std::string ret_ant = test_radio->get_rx_antenna(chan);
            BOOST_CHECK_EQUAL(iter, ret_ant);
        }
    }
    log = "ZBX TX ANTENNA TEST";
    for (auto fe_path :
        {fs_path("dboard/tx_frontends/0"), fs_path("dboard/tx_frontends/1")}) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " ANTENNA CHANGE\n");
        for (auto iter : TX_ANTENNAS) {
            UHD_LOG_INFO(log, "Testing Antenna: " << iter);

            tree->access<std::string>(fe_path / "antenna/value").set(iter);

            std::string ret_ant =
                tree->access<std::string>(fe_path / "antenna/value").get();
            BOOST_CHECK_EQUAL(iter, ret_ant);
        }
    }
    for (size_t chan = 0; chan < 2; chan++) {
        for (auto iter : TX_ANTENNAS) {
            UHD_LOG_INFO(log, "Testing Antenna: " << iter);

            test_radio->set_tx_antenna(iter, chan);

            std::string ret_ant = test_radio->get_tx_antenna(chan);
            BOOST_CHECK_EQUAL(iter, ret_ant);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_freq_coercion_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX_FREQUENCY_COERCION_TEST";
    const double ep       = 10;

    for (auto fe_path : {
             fs_path("dboard/tx_frontends/0"),
             fs_path("dboard/tx_frontends/1"),
             fs_path("dboard/rx_frontends/0"),
             fs_path("dboard/rx_frontends/1"),
         }) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " FREQUENCY COERCION\n");
        double ret_value =
            tree->access<double>(fe_path / "freq").set(ZBX_MIN_FREQ - 1e6).get();

        BOOST_REQUIRE(abs(ZBX_MIN_FREQ - ret_value) < ep);

        ret_value = tree->access<double>(fe_path / "freq").set(ZBX_MAX_FREQ + 1e6).get();

        BOOST_REQUIRE(abs(ZBX_MAX_FREQ - ret_value) < ep);
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_tx_gain_coercion_test, x400_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX_GAIN_COERCION_TEST";

    for (auto fe_path :
        {fs_path("dboard/tx_frontends/0"), fs_path("dboard/tx_frontends/1")}) {
        uhd::gain_range_t zbx_gain(TX_MIN_GAIN, TX_MAX_GAIN, 0.1);
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " TX GAIN COERCION\n");
        for (double iter = zbx_gain.start(); iter <= zbx_gain.stop();
             iter += zbx_gain.step()) {
            const auto gain_path = fe_path / "gains" / ZBX_GAIN_STAGE_ALL / "value";
            const double ret_val = tree->access<double>(gain_path).set(iter).get();
            BOOST_CHECK_EQUAL(ret_val, std::round(iter));
        }
    }
    for (auto fe_path :
        {fs_path("dboard/rx_frontends/0"), fs_path("dboard/rx_frontends/1")}) {
        uhd::gain_range_t zbx_gain(RX_MIN_GAIN, RX_MAX_GAIN, 0.1);
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " RX GAIN COERCION\n");
        for (double iter = zbx_gain.start(); iter <= zbx_gain.stop();
             iter += zbx_gain.step()) {
            const auto gain_path = fe_path / "gains" / ZBX_GAIN_STAGE_ALL / "value";
            const double ret_val = tree->access<double>(gain_path).set(iter).get();
            BOOST_CHECK_EQUAL(ret_val, std::round(iter));
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_phase_sync_test, x400_radio_fixture)
{
    auto tree                        = test_radio->get_tree();
    const std::string log            = "ZBX_PHASE_SYNC_TEST";
    constexpr uint32_t lo_sync_addr  = 0x1024 + 0x80000;
    constexpr uint32_t nco_sync_addr = 0x88000;
    constexpr uint32_t gearbox_addr  = 0x88004;
    auto& regs                       = reg_iface->read_memory;
    UHD_LOG_INFO("TEST", "Setting 1 GHz defaults...");
    // Confirm default
    test_radio->set_rx_frequency(1e9, 0);
    test_radio->set_rx_frequency(1e9, 1);
    test_radio->set_tx_frequency(1e9, 0);
    test_radio->set_tx_frequency(1e9, 1);
    // Enable time stamp
    UHD_LOG_INFO("TEST", "Enabling time stamp chan 0...");
    test_radio->set_command_time(uhd::time_spec_t(2.0), 0);
    // Don't pick the ZBX default frequency here
    UHD_LOG_INFO("TEST", "Setting RX chan 0 to 2.3 GHz...");
    test_radio->set_rx_frequency(2.3e9, 0);
    // Check we synced RX LOs chan 0 and RX NCO chan 0, and ADC gearboxes
    BOOST_CHECK_EQUAL(regs[lo_sync_addr], 0b11 << 4);
    BOOST_CHECK_EQUAL(regs[nco_sync_addr], 1);
    BOOST_CHECK_EQUAL(regs[gearbox_addr], 1);
    // Reset strobes
    regs[lo_sync_addr]  = 0;
    regs[nco_sync_addr] = 0;
    regs[gearbox_addr]  = 0;
    UHD_LOG_INFO("TEST", "Enabling time stamp chan 1...");
    test_radio->set_command_time(uhd::time_spec_t(2.0), 1);
    UHD_LOG_INFO("TEST", "Setting RX chan 1 to 2.3 GHz...");
    test_radio->set_rx_frequency(2.3e9, 1);
    // Check we synced RX LOs chan 1 and RX NCO chan 1. ADC gearbox only gets
    // reset once, and should be left untouched.
    BOOST_CHECK_EQUAL(regs[lo_sync_addr], 0b11 << 6);
    BOOST_CHECK_EQUAL(regs[nco_sync_addr], 1);
    BOOST_CHECK_EQUAL(regs[gearbox_addr], 0);
    // Reset strobes
    regs[lo_sync_addr]  = 0;
    regs[nco_sync_addr] = 0;
    regs[gearbox_addr]  = 0;
    UHD_LOG_INFO("TEST", "Setting TX chan 0 to 2.3 GHz...");
    test_radio->set_tx_frequency(2.3e9, 0);
    // Check we synced TX LOs chan 0 and TX NCO chan 0, and DAC gearboxes
    BOOST_CHECK_EQUAL(regs[lo_sync_addr], 0x3 << 0);
    BOOST_CHECK_EQUAL(regs[nco_sync_addr], 1);
    BOOST_CHECK_EQUAL(regs[gearbox_addr], 1 << 1);
    // Reset strobe
    regs[lo_sync_addr]  = 0;
    regs[nco_sync_addr] = 0;
    regs[gearbox_addr]  = 0;
    UHD_LOG_INFO("TEST", "Setting TX chan 1 to 2.3 GHz...");
    test_radio->set_tx_frequency(2.3e9, 1);
    // Check we synced TX LOs chan 1 and TX NCO chan 1. DAC gearbox only gets
    // reset once, and should be left untouched.
    BOOST_CHECK_EQUAL(regs[lo_sync_addr], 0xC << 0);
    BOOST_CHECK_EQUAL(regs[nco_sync_addr], 1);
    BOOST_CHECK_EQUAL(regs[gearbox_addr], 0);
    // Reset strobe
    regs[lo_sync_addr]  = 0;
    regs[nco_sync_addr] = 0;
    regs[gearbox_addr]  = 0;
}

BOOST_FIXTURE_TEST_CASE(can_set_rfdc_test, x400_radio_fixture)
{
    test_radio->set_tx_lo_freq(3.141e9, "rfdc", 1);
    test_radio->get_tx_lo_freq("rfdc", 1);

    test_radio->set_rx_lo_freq(2.141e9, "rfdc", 0);
    test_radio->get_rx_lo_freq("rfdc", 0);
}

BOOST_FIXTURE_TEST_CASE(zbx_tx_power_api, x400_radio_fixture)
{
    constexpr double tx_given_gain  = 30;
    constexpr double tx_given_power = -30;

    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX_TX_POWER_TRACKING_TEST";
    auto tx_pwr_mgr       = test_radio->get_pwr_mgr(TX_DIRECTION);

    for (size_t chan = 0; chan < ZBX_NUM_CHANS; chan++) {
        // Start in gain tracking mode
        double gain_coerced = test_radio->set_tx_gain(tx_given_gain, chan);
        BOOST_CHECK_EQUAL(gain_coerced, tx_given_gain);
        for (const double freq : {6e+08, 1e+09, 2e+09, 3e+09, 4e+09, 5e+09, 6e+09}) {
            // Setting a power reference should kick us into power tracking mode
            test_radio->set_tx_power_reference(tx_given_power, chan);

            test_radio->set_tx_frequency(freq, chan);
            // If the tracking mode is properly set, we should not deviate much
            // regarding power
            const double pow_diff =
                std::abs(tx_given_power - test_radio->get_tx_power_reference(chan));
            BOOST_CHECK_MESSAGE(
                pow_diff < 3.0, "power differential is too large: " << pow_diff);

            // Back to gain mode
            gain_coerced = test_radio->set_tx_gain(tx_given_gain, chan);
            BOOST_CHECK_EQUAL(gain_coerced, tx_given_gain);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_rx_power_api, x400_radio_fixture)
{
    constexpr double rx_given_gain  = 30;
    constexpr double rx_given_power = -30;

    auto tree             = test_radio->get_tree();
    const std::string log = "ZBX_RX_POWER_TRACKING_TEST";
    auto rx_pwr_mgr       = test_radio->get_pwr_mgr(RX_DIRECTION);

    for (size_t chan = 0; chan < ZBX_NUM_CHANS; chan++) {
        // Start in gain tracking mode
        double gain_coerced = test_radio->set_rx_gain(rx_given_gain, chan);
        BOOST_REQUIRE_EQUAL(gain_coerced, rx_given_gain);
        for (const double freq : {1e+09, 2e+09, 3e+09, 4e+09, 5e+09, 6e+09}) {
            // Setting a power reference should kick us into power tracking mode
            test_radio->set_rx_power_reference(rx_given_power, chan);
            // Now go tune
            test_radio->set_rx_frequency(freq, chan);
            // If the tracking mode is properly set, we should match our expected criteria
            // for power reference levels
            const double actual_power = test_radio->get_rx_power_reference(chan);
            const double pow_diff     = std::abs(rx_given_power - actual_power);
            BOOST_CHECK_MESSAGE(pow_diff < 3.0,
                "power differential is too large ("
                    << pow_diff << "): Expected close to: " << rx_given_power
                    << " Actual: " << actual_power << " Frequency: " << (freq / 1e6));

            gain_coerced = test_radio->set_rx_gain(rx_given_gain, chan);
            BOOST_REQUIRE_EQUAL(gain_coerced, rx_given_gain);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_tx_lo_injection_locking, x400_radio_fixture)
{
    auto tree = test_radio->get_tree();

    // As of right now, we don't have a way to directly get the DB prc rate, this is the
    // value of the prc map per DEFAULT_MCR, in the mock RPC server:db_0_get_db_prc_rate()
    constexpr double db_prc_rate  = 61.44e6;
    constexpr double lo_step_size = db_prc_rate / ZBX_RELATIVE_LO_STEP_SIZE;

    uhd::freq_range_t zbx_freq(ZBX_MIN_FREQ, ZBX_MAX_FREQ, 100e6);

    for (double iter = zbx_freq.start(); iter <= zbx_freq.stop();
         iter += zbx_freq.step()) {
        for (const size_t chan : {0, 1}) {
            test_radio->set_tx_frequency(iter, chan);

            // The step alignment only applies to the desired LO frequency, the actual
            // returned frequency may vary slightly
            const double lo1_freq = std::round(test_radio->get_tx_lo_freq(ZBX_LO1, chan));
            const double lo2_freq = std::round(test_radio->get_tx_lo_freq(ZBX_LO2, chan));

            const double lo1_div = lo1_freq / lo_step_size;
            const double lo2_div = lo2_freq / lo_step_size;

            // Test whether our tuned frequencies align with the lo step size
            BOOST_CHECK_EQUAL(std::floor(lo1_div), lo1_div);
            BOOST_CHECK_EQUAL(std::floor(lo2_div), lo2_div);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_rx_lo_injection_locking, x400_radio_fixture)
{
    auto tree = test_radio->get_tree();

    // As of right now, we don't have a way to directly get the DB prc rate, this is the
    // value of the prc map per DEFAULT_MCR, in the mock RPC server:db_0_get_db_prc_rate()
    constexpr double db_prc_rate  = 61.44e6;
    constexpr double lo_step_size = db_prc_rate / ZBX_RELATIVE_LO_STEP_SIZE;

    uhd::freq_range_t zbx_freq(ZBX_MIN_FREQ, ZBX_MAX_FREQ, 100e6);

    for (double iter = zbx_freq.start(); iter <= zbx_freq.stop();
         iter += zbx_freq.step()) {
        for (const size_t chan : {0, 1}) {
            test_radio->set_rx_frequency(iter, chan);

            // The step alignment only applies to the desired LO frequency, the actual
            // returned frequency may vary slightly
            const double lo1_freq = std::round(test_radio->get_rx_lo_freq(ZBX_LO1, chan));
            const double lo2_freq = std::round(test_radio->get_rx_lo_freq(ZBX_LO2, chan));

            const double lo1_div = lo1_freq / lo_step_size;
            const double lo2_div = lo2_freq / lo_step_size;

            // Test whether our tuned frequencies align with the lo step size
            BOOST_CHECK_EQUAL(std::floor(lo1_div), lo1_div);
            BOOST_CHECK_EQUAL(std::floor(lo2_div), lo2_div);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(zbx_rx_gain_profile_test, x400_radio_fixture)
{
    auto tree                         = test_radio->get_tree();
    const std::string log             = "ZBX_GAIN_PROFILE_TEST";
    auto& regs                        = reg_iface->read_memory;
    constexpr uint32_t current_config = radio_control_impl::regmap::PERIPH_BASE + 0x1000;
    constexpr uint32_t rf_option      = radio_control_impl::regmap::PERIPH_BASE + 0x1004;
    constexpr uint32_t sw_config      = radio_control_impl::regmap::PERIPH_BASE + 0x1008;
    constexpr uint32_t rx0_dsa        = radio_control_impl::regmap::PERIPH_BASE + 0x3800;
    constexpr uint32_t rx0_table      = radio_control_impl::regmap::PERIPH_BASE + 0x5800;
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain_profile(0), "default");
    BOOST_CHECK_EQUAL(test_radio->get_tx_gain_profile(0), "default");
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain_profile(1), "default");
    BOOST_CHECK_EQUAL(test_radio->get_tx_gain_profile(1), "default");
    // Everything should be classic_atr
    BOOST_CHECK_EQUAL(regs[0x81004], 0x01010101);
    // Can't set gain stages in this profile
    BOOST_REQUIRE_THROW(test_radio->set_rx_gain(10, "DSA1", 0), uhd::key_error);
    BOOST_REQUIRE_THROW(test_radio->set_tx_gain(10, "DSA1", 0), uhd::key_error);

    //** manual gain profile **
    test_radio->set_rx_gain_profile("manual", 0);
    // Must provide valid gain name in this profile
    BOOST_REQUIRE_THROW(test_radio->set_rx_gain(23, 0), uhd::runtime_error);
    BOOST_REQUIRE_THROW(test_radio->set_rx_gain(10, "banana", 0), uhd::key_error);
    // Now manually set the DSAs
    BOOST_CHECK_EQUAL(5, test_radio->set_rx_gain(5, "DSA1", 0));
    BOOST_CHECK_EQUAL(5, test_radio->set_rx_gain(5, "DSA2", 0));
    BOOST_CHECK_EQUAL(5, test_radio->set_rx_gain(5, "DSA3A", 0));
    BOOST_CHECK_EQUAL(5, test_radio->set_rx_gain(5, "DSA3B", 0));
    // Check the registers were written to correctly (gain 5 == att 10)
    BOOST_CHECK_EQUAL(regs[rx0_dsa + 1 * 4], 0xAAAA);
    BOOST_CHECK_EQUAL(regs[rx0_dsa + 3 * 4], 0xAAAA);
    // Check the getters:
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain("DSA1", 0), 5);
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain("DSA2", 0), 5);
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain("DSA3A", 0), 5);
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain("DSA3B", 0), 5);
    // Even in 'manual', we can load from the table. Let's create a table entry:
    regs[rx0_table + 5 * 4] = 0x7777;
    // Now, let it be loaded into RX and XX:
    BOOST_CHECK_EQUAL(5, test_radio->set_rx_gain(5, "TABLE", 0));
    BOOST_CHECK_EQUAL(regs[rx0_dsa + 1 * 4], 0x7777);
    BOOST_CHECK_EQUAL(regs[rx0_dsa + 3 * 4], 0x7777);
    // Note: If we read back the DSAs via get_rx_gain() now, they will still say
    // 5. We might want to change that, but it will require extra peeks. The
    // only good way to do that is to amend set_?x_gain() to do that peek when
    // updating gains via table.
    // Test DSA coercion
    BOOST_CHECK_EQUAL(15, test_radio->set_rx_gain(39, "DSA1", 0));
    BOOST_CHECK_EQUAL(0, test_radio->set_rx_gain(-17, "DSA1", 0));

    // If we go back to 'default', we also reset the DSAs. That's because the
    // desired, previously loaded default value will trigger the previous DSA
    // values again.
    UHD_LOG_INFO(log, "resetting to default");
    test_radio->set_rx_gain_profile("default", 0);
    BOOST_CHECK_EQUAL(0, test_radio->get_rx_gain("DSA1", 0));

    //** table_noatr profile : **
    UHD_LOG_INFO(log, "setting to table_noatr");
    test_radio->set_rx_gain_profile("table_noatr", 0);
    // This will set DSA config for chan 0 to 0 == SW_DEFINED
    BOOST_CHECK_EQUAL(regs[rf_option], 0x01000101);
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain_profile(0), "table_noatr");
    // Yup, this will also change TX gain profile; they're coupled.
    BOOST_CHECK_EQUAL(test_radio->get_tx_gain_profile(0), "table_noatr");
    BOOST_REQUIRE_THROW(test_radio->set_rx_gain(10, "all", 0), uhd::key_error);
    BOOST_CHECK_EQUAL(8.0, test_radio->set_rx_gain(8, "TABLE", 0));
    BOOST_CHECK_EQUAL(regs[sw_config], 0x80000);
    // Returns the current config. Note the asymmetry to the previous API call.
    // We can't, however, know which entry from the TABLE we used, so we just
    // return the current config (which is the entry from the DSA table, not the
    // TABLE it writes to).
    BOOST_CHECK_EQUAL(0, test_radio->get_rx_gain("TABLE", 0));
    // Let's pretend we're using config 7
    regs[current_config] = 0x70000;
    BOOST_CHECK_EQUAL(7, test_radio->get_rx_gain("TABLE", 0));
    // And back
    regs[current_config] = 0x00000;
    // Now we fake an FPGA-gain-change transaction that UHD is unaware of. We
    // keep the current config of 0, and update RX0_DSA*[0].
    regs[rx0_dsa + 0 * 4] = 0x4444; // Turn it up to attenuation 4 == gain 11
    BOOST_CHECK_EQUAL(11.0, test_radio->get_rx_gain("DSA1", 0));

    //** table profile **
    test_radio->set_rx_gain_profile("table", 0);
    BOOST_CHECK_EQUAL(regs[rf_option], 0x01010101);
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain_profile(0), "table");
    // Yup, this will also change TX gain profile; they're coupled.
    BOOST_CHECK_EQUAL(test_radio->get_tx_gain_profile(0), "table");
    // Create another table entry
    regs[rx0_table + 23 * 4] = 0xBBBB;
    BOOST_CHECK_EQUAL(23.0, test_radio->set_rx_gain(23, "TABLE", 0));
    // get_rx_gain() for "TABLE" returns the current DSA table index, not actual gain
    BOOST_CHECK_EQUAL(0.0, test_radio->get_rx_gain("TABLE", 0));
    // This will update RX and XX registers (that's the difference to table_noatr)
    BOOST_CHECK_EQUAL(regs[rx0_dsa + 1 * 4], 0xBBBB); // att 0xB == gain 4.0
    BOOST_CHECK_EQUAL(regs[rx0_dsa + 3 * 4], 0xBBBB);
    BOOST_CHECK_EQUAL(4.0, test_radio->get_rx_gain("DSA1", 0));
    BOOST_CHECK_EQUAL(4.0, test_radio->get_rx_gain("DSA2", 0));
    BOOST_CHECK_EQUAL(4.0, test_radio->get_rx_gain("DSA3A", 0));
    BOOST_CHECK_EQUAL(4.0, test_radio->get_rx_gain("DSA3B", 0));
    // Test table coercion
    UHD_LOG_INFO(log, "Testing TABLE coercion");
    BOOST_CHECK_EQUAL(0.0, test_radio->set_rx_gain(-17, "TABLE", 0));
    BOOST_CHECK_EQUAL(255.0, test_radio->set_rx_gain(1e9, "TABLE", 0));
}

BOOST_FIXTURE_TEST_CASE(zbx_tx_gain_profile_test, x400_radio_fixture)
{
    auto tree                         = test_radio->get_tree();
    const std::string log             = "ZBX_GAIN_PROFILE_TEST";
    auto& regs                        = reg_iface->read_memory;
    constexpr uint32_t current_config = radio_control_impl::regmap::PERIPH_BASE + 0x1000;
    constexpr uint32_t rf_option      = radio_control_impl::regmap::PERIPH_BASE + 0x1004;
    constexpr uint32_t sw_config      = radio_control_impl::regmap::PERIPH_BASE + 0x1008;
    constexpr uint32_t tx0_dsa        = radio_control_impl::regmap::PERIPH_BASE + 0x3000;
    constexpr uint32_t tx0_table      = radio_control_impl::regmap::PERIPH_BASE + 0x5000;
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain_profile(0), "default");
    BOOST_CHECK_EQUAL(test_radio->get_tx_gain_profile(0), "default");
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain_profile(1), "default");
    BOOST_CHECK_EQUAL(test_radio->get_tx_gain_profile(1), "default");
    const double default_dsa1 = test_radio->get_tx_gain("DSA1", 0);
    // Everything should be classic_atr
    BOOST_CHECK_EQUAL(regs[0x81004], 0x01010101);
    // Can't set gain stages in this profile
    BOOST_REQUIRE_THROW(test_radio->set_rx_gain(10, "DSA1", 0), uhd::key_error);
    BOOST_REQUIRE_THROW(test_radio->set_tx_gain(10, "DSA1", 0), uhd::key_error);

    //** manual gain profile **
    test_radio->set_tx_gain_profile("manual", 0);
    // Must provide valid gain name in this profile
    BOOST_REQUIRE_THROW(test_radio->set_tx_gain(23, 0), uhd::runtime_error);
    BOOST_REQUIRE_THROW(test_radio->set_tx_gain(23, "all", 0), uhd::key_error);
    BOOST_REQUIRE_THROW(test_radio->set_tx_gain(10, "banana", 0), uhd::key_error);
    // Now manually set the DSAs
    BOOST_CHECK_EQUAL(21, test_radio->set_tx_gain(21, "DSA1", 0));
    BOOST_CHECK_EQUAL(21, test_radio->set_tx_gain(21, "DSA2", 0));
    // Check the registers were written to correctly (gain 5 == att 10)
    BOOST_CHECK_EQUAL(regs[tx0_dsa + 2 * 4], 0x0A0A);
    BOOST_CHECK_EQUAL(regs[tx0_dsa + 3 * 4], 0x0A0A);
    // Check the getters:
    BOOST_CHECK_EQUAL(test_radio->get_tx_gain("DSA1", 0), 21);
    BOOST_CHECK_EQUAL(test_radio->get_tx_gain("DSA2", 0), 21);
    // Even in 'manual', we can load from the table. Let's create a table entry:
    regs[tx0_table + 5 * 4] = 0x0707;
    // Now, let it be loaded into RX and XX:
    BOOST_CHECK_EQUAL(5, test_radio->set_tx_gain(5, "TABLE", 0));
    BOOST_CHECK_EQUAL(regs[tx0_dsa + 2 * 4], 0x0707);
    BOOST_CHECK_EQUAL(regs[tx0_dsa + 3 * 4], 0x0707);
    // Note: If we read back the DSAs via get_tx_gain() now, they will still say
    // 5. We might want to change that, but it will require extra peeks. The
    // only good way to do that is to amend set_?x_gain() to do that peek when
    // updating gains via table.
    // Test DSA coercion
    BOOST_CHECK_EQUAL(31, test_radio->set_tx_gain(39, "DSA1", 0));
    BOOST_CHECK_EQUAL(0, test_radio->set_tx_gain(-17, "DSA1", 0));

    // If we go back to 'default', we also reset the DSAs. That's because the
    // desired, previously loaded default value will trigger the previous DSA
    // values again.
    UHD_LOG_INFO(log, "resetting to default");
    test_radio->set_tx_gain_profile("default", 0);
    BOOST_CHECK_EQUAL(default_dsa1, test_radio->get_tx_gain("DSA1", 0));

    //** table_noatr profile : **
    UHD_LOG_INFO(log, "setting to table_noatr");
    test_radio->set_tx_gain_profile("table_noatr", 0);
    // This will set DSA config for chan 0 to 0 == SW_DEFINED
    BOOST_CHECK_EQUAL(regs[rf_option], 0x01000101);
    BOOST_CHECK_EQUAL(test_radio->get_tx_gain_profile(0), "table_noatr");
    // Yup, this will also change RX gain profile; they're coupled.
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain_profile(0), "table_noatr");
    BOOST_REQUIRE_THROW(test_radio->set_tx_gain(10, "all", 0), uhd::key_error);
    BOOST_CHECK_EQUAL(8.0, test_radio->set_tx_gain(8, "TABLE", 0));
    BOOST_CHECK_EQUAL(regs[sw_config], 0x80000);
    // Returns the current config. Note the asymmetry to the previous API call.
    // We can't, however, know which entry from the TABLE we used, so we just
    // return the current config (which is the entry from the DSA table, not the
    // TABLE it writes to).
    BOOST_CHECK_EQUAL(0, test_radio->get_tx_gain("TABLE", 0));
    // Let's pretend we're using config 7
    regs[current_config] = 0x70000;
    BOOST_CHECK_EQUAL(7, test_radio->get_tx_gain("TABLE", 0));
    // And back
    regs[current_config] = 0x00000;
    // Now we fake an FPGA-gain-change transaction that UHD is unaware of. We
    // keep the current config of 0, and update TX0_DSA*[0].
    regs[tx0_dsa + 0 * 4] = 0x0404; // Turn it up to attenuation 4 == gain 27
    BOOST_CHECK_EQUAL(27.0, test_radio->get_tx_gain("DSA1", 0));

    //** table profile **
    test_radio->set_tx_gain_profile("table", 0);
    BOOST_CHECK_EQUAL(regs[rf_option], 0x01010101);
    BOOST_CHECK_EQUAL(test_radio->get_tx_gain_profile(0), "table");
    // Yup, this will also change RX gain profile; they're coupled.
    BOOST_CHECK_EQUAL(test_radio->get_rx_gain_profile(0), "table");
    // Create another table entry
    regs[tx0_table + 23 * 4] = 0x0B0B;
    BOOST_CHECK_EQUAL(23.0, test_radio->set_tx_gain(23, "TABLE", 0));
    // get_tx_gain() for "TABLE" returns the current DSA table index, not actual gain
    BOOST_CHECK_EQUAL(0.0, test_radio->get_tx_gain("TABLE", 0));
    // This will update RX and XX registers (that's the difference to table_noatr)
    BOOST_CHECK_EQUAL(regs[tx0_dsa + 2 * 4], 0x0B0B); // att 0xB == gain 20.0
    BOOST_CHECK_EQUAL(regs[tx0_dsa + 3 * 4], 0x0B0B);
    BOOST_CHECK_EQUAL(20.0, test_radio->get_tx_gain("DSA1", 0));
    BOOST_CHECK_EQUAL(20.0, test_radio->get_tx_gain("DSA2", 0));
    // Test table coercion
    UHD_LOG_INFO(log, "Testing TABLE coercion");
    BOOST_CHECK_EQUAL(0.0, test_radio->set_tx_gain(-17, "TABLE", 0));
    BOOST_CHECK_EQUAL(255.0, test_radio->set_tx_gain(1e9, "TABLE", 0));
}

// TODO:
// - concurrent/consecutive configuration
// - Threading tests
// - Error cases

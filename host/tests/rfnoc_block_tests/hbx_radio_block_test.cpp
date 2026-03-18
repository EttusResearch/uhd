//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "hbx_radio_mock.hpp"
#include "x4xx_hbx_mpm_mock.hpp"
#include <uhd/cal/database.hpp>
#include <uhd/cal/pwr_cal.hpp>
#include <uhd/rfnoc/detail/graph.hpp>
#include <uhd/rfnoc/mock_block.hpp>
#include <uhd/rfnoc/mock_nodes.hpp>
#include <uhd/types/iq_dc_cal_coeffs.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/paths.hpp>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <thread>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::test;
using namespace std::chrono_literals;
using namespace uhd::usrp::hbx;
using namespace uhd::experts;

namespace {

std::string _sanitize_antenna_name(std::string antenna)
{
    std::replace(antenna.begin(), antenna.end(), '/', '+');
    std::transform(antenna.begin(), antenna.end(), antenna.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return antenna;
}

void _seed_hbx_mock_power_cal_data(x400_hbx_radio_fixture& fixture)
{
    namespace fs = std::filesystem;

    static bool seeded = false;
    if (seeded) {
        return;
    }

    const fs::path cal_path = fs::path(uhd::get_tmp_path()) / "UHD_HBX_MOCK_CAL";
    std::error_code ec;
    fs::create_directories(cal_path, ec);
    if (ec) {
        BOOST_FAIL("Failed to create temporary cal path: " << cal_path.string());
    }

#ifdef UHD_PLATFORM_WIN32
    const std::string putenv_str = std::string("UHD_CAL_DATA_PATH=") + cal_path.string();
    _putenv(putenv_str.c_str());
#else
    setenv("UHD_CAL_DATA_PATH", cal_path.string().c_str(), 1);
#endif

    const std::string serial = "HBX_MOCK_NO_CAL#HBX";
    const std::string tx_key =
        "x4xx_pwr_hbx_tx_0_"
        + _sanitize_antenna_name(fixture.test_radio->get_tx_antenna(0));
    const std::string rx_key =
        "x4xx_pwr_hbx_rx_0_"
        + _sanitize_antenna_name(fixture.test_radio->get_rx_antenna(0));

    const auto make_cal_blob = [serial]() {
        auto cal_data =
            uhd::usrp::cal::pwr_cal::make("HBX mock power cal", serial, 0x12345678);
        std::map<double, double> gain_to_power;
        for (int gain = 0; gain <= 80; gain++) {
            gain_to_power[static_cast<double>(gain)] = static_cast<double>(gain) - 60.0;
        }
        for (const double freq : {1e9, 3e9, 6e9, 10e9}) {
            cal_data->add_power_table(gain_to_power, -60.0, 20.0, freq);
        }
        return cal_data->serialize();
    };

    uhd::usrp::cal::database::write_cal_data(tx_key, serial, make_cal_blob());
    uhd::usrp::cal::database::write_cal_data(rx_key, serial, make_cal_blob());
    seeded = true;
}

} // namespace

BOOST_FIXTURE_TEST_CASE(hbx_api_freq_tx_test, x400_hbx_radio_fixture)
{
    const std::string log = "HBX_API_TX_FREQUENCY_TEST";
    const double ep       = 10;
    // HBX supports 10 MHz to 20 GHz
    uhd::freq_range_t hbx_freq(HBX_MIN_FREQ, HBX_MAX_FREQ, 500e6);

    // HBX has only 1 channel
    for (const size_t chan : {0}) {
        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " FREQ CHANGE (SET->RETURN)\n");
        for (double iter = hbx_freq.start(); iter <= hbx_freq.stop();
             iter += hbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            const double freq = test_radio->set_tx_frequency(iter, chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }

        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " FREQ CHANGE (SET->GET)\n");
        for (double iter = hbx_freq.start(); iter <= hbx_freq.stop();
             iter += hbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            test_radio->set_tx_frequency(iter, chan);
            const double freq = test_radio->get_tx_frequency(chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_api_freq_rx_test, x400_hbx_radio_fixture)
{
    const std::string log = "HBX_API_RX_FREQUENCY_TEST";
    const double ep       = 10;
    // HBX supports 10 MHz to 20 GHz
    uhd::freq_range_t hbx_freq(HBX_MIN_FREQ, HBX_MAX_FREQ, 500e6);

    // HBX has only 1 channel
    for (const size_t chan : {0}) {
        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " FREQ CHANGE (SET->RETURN)\n");
        for (double iter = hbx_freq.start(); iter <= hbx_freq.stop();
             iter += hbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            const double freq = test_radio->set_rx_frequency(iter, chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }
        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " FREQ CHANGE (SET->GET)\n");
        for (double iter = hbx_freq.start(); iter <= hbx_freq.stop();
             iter += hbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            test_radio->set_rx_frequency(iter, chan);
            const double freq = test_radio->get_rx_frequency(chan);
            BOOST_REQUIRE(abs(iter - freq) < ep);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_frequency_test, x400_hbx_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "HBX_FREQUENCY_TEST";
    const double ep       = 10;
    // HBX supports 10 MHz to 20 GHz
    uhd::freq_range_t hbx_freq(HBX_MIN_FREQ, HBX_MAX_FREQ, 500e6);

    for (const auto& fe_path : {
             fs_path("dboard/tx_frontends/0"),
             fs_path("dboard/rx_frontends/0"),
         }) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " FREQ CHANGE\n");
        for (double iter = hbx_freq.start(); iter <= hbx_freq.stop();
             iter += hbx_freq.step()) {
            UHD_LOG_INFO(log, "Testing freq: " << iter);

            tree->access<double>(fe_path / "freq").set(iter);

            const double ret_value = tree->access<double>(fe_path / "freq").get();

            BOOST_REQUIRE(abs(iter - ret_value) < ep);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_api_tx_gain_test, x400_hbx_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "HBX_API_TX_GAIN_TEST";
    const auto gain_range = test_radio->get_tx_gain_range(0);
    uhd::gain_range_t hbx_gain(gain_range.start(), gain_range.stop(), gain_range.step());

    for (const size_t chan : {0}) {
        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " GAIN CHANGE (SET->RETURN)\n");
        for (double iter = hbx_gain.start(); iter <= hbx_gain.stop();
             iter += hbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);
            const double ret_gain = test_radio->set_tx_gain(iter, chan);
            BOOST_CHECK_EQUAL(iter, ret_gain);
        }

        UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " GAIN CHANGE (SET->GET)\n");
        for (double iter = hbx_gain.start(); iter <= hbx_gain.stop();
             iter += hbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);
            test_radio->set_tx_gain(iter, chan);
            const double ret_gain = test_radio->get_tx_gain(chan);
            BOOST_CHECK_EQUAL(iter, ret_gain);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_api_rx_gain_test, x400_hbx_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "HBX_API_RX_GAIN_TEST";
    const auto gain_range = test_radio->get_rx_gain_range(0);
    uhd::gain_range_t hbx_gain(gain_range.start(), gain_range.stop(), gain_range.step());

    for (const size_t chan : {0}) {
        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " GAIN CHANGE (SET->RETURN)\n");
        for (double iter = hbx_gain.start(); iter <= hbx_gain.stop();
             iter += hbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);
            const double ret_gain = test_radio->set_rx_gain(iter, chan);
            BOOST_CHECK_EQUAL(iter, ret_gain);
        }

        UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " GAIN CHANGE (SET->GET)\n");
        for (double iter = hbx_gain.start(); iter <= hbx_gain.stop();
             iter += hbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);
            test_radio->set_rx_gain(iter, chan);
            const double ret_gain = test_radio->get_rx_gain(chan);
            BOOST_CHECK_EQUAL(iter, ret_gain);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_tx_power_api, x400_hbx_radio_fixture)
{
    _seed_hbx_mock_power_cal_data(*this);

    const std::string log           = "HBX_TX_POWER_TRACKING_TEST";
    const size_t chan               = 0;
    const auto gain_range           = test_radio->get_tx_gain_range(chan);
    const double tx_given_gain      = (gain_range.start() + gain_range.stop()) / 2.0;
    constexpr double tx_given_power = -30.0;

    UHD_LOG_INFO(log, "BEGIN TEST: tx" << chan << " POWER TRACKING API\n");
    BOOST_REQUIRE_EQUAL(test_radio->set_tx_gain(tx_given_gain, chan), tx_given_gain);

    for (const double freq : {1e9, 3e9, 6e9, 10e9}) {
        BOOST_REQUIRE_NO_THROW(test_radio->set_tx_power_reference(tx_given_power, chan));

        test_radio->set_tx_frequency(freq, chan);

        const double actual_power = test_radio->get_tx_power_reference(chan);
        const double pow_diff     = std::abs(tx_given_power - actual_power);
        BOOST_CHECK_MESSAGE(pow_diff < 3.0,
            "TX power differential is too large ("
                << pow_diff << "): expected close to " << tx_given_power << " actual "
                << actual_power << " frequency " << (freq / 1e6) << " MHz");

        BOOST_REQUIRE_EQUAL(test_radio->set_tx_gain(tx_given_gain, chan), tx_given_gain);
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_rx_power_api, x400_hbx_radio_fixture)
{
    _seed_hbx_mock_power_cal_data(*this);

    const std::string log           = "HBX_RX_POWER_TRACKING_TEST";
    const size_t chan               = 0;
    const auto gain_range           = test_radio->get_rx_gain_range(chan);
    const double rx_given_gain      = (gain_range.start() + gain_range.stop()) / 2.0;
    constexpr double rx_given_power = -30.0;

    UHD_LOG_INFO(log, "BEGIN TEST: rx" << chan << " POWER TRACKING API\n");
    BOOST_REQUIRE_EQUAL(test_radio->set_rx_gain(rx_given_gain, chan), rx_given_gain);

    for (const double freq : {1e9, 3e9, 6e9, 10e9}) {
        BOOST_REQUIRE_NO_THROW(test_radio->set_rx_power_reference(rx_given_power, chan));

        test_radio->set_rx_frequency(freq, chan);

        const double actual_power = test_radio->get_rx_power_reference(chan);
        const double pow_diff     = std::abs(rx_given_power - actual_power);
        BOOST_CHECK_MESSAGE(pow_diff < 3.0,
            "RX power differential is too large ("
                << pow_diff << "): expected close to " << rx_given_power << " actual "
                << actual_power << " frequency " << (freq / 1e6) << " MHz");

        BOOST_REQUIRE_EQUAL(test_radio->set_rx_gain(rx_given_gain, chan), rx_given_gain);
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_api_tx_gain_stage_test, x400_hbx_radio_fixture)
{
    const std::string log = "HBX_API_TX_GAIN_STAGE_TEST";

    for (const size_t chan : {0}) {
        test_radio->set_tx_gain_profile(HBX_GAIN_PROFILE_MANUAL, chan);

        UHD_LOG_INFO(
            log, "BEGIN TEST: tx" << chan << " GAIN STAGE CHANGE (SET->RETURN)\n");
        for (const auto& gain_stage : HBX_TX_GAIN_STAGES_MANUAL) {
            const auto gain_range   = test_radio->get_tx_gain_range(gain_stage, chan);
            const double prime_gain = (gain_range.start() == gain_range.stop())
                                          ? gain_range.start()
                                          : gain_range.stop();
            test_radio->set_tx_gain(prime_gain, gain_stage, chan);
            if (gain_range.step() == 0.0) {
                const double ret_gain =
                    test_radio->set_tx_gain(gain_range.start(), gain_stage, chan);
                BOOST_CHECK_EQUAL(gain_range.start(), ret_gain);
                continue;
            }
            for (double iter = gain_range.start(); iter <= gain_range.stop();
                 iter += gain_range.step()) {
                UHD_LOG_INFO(log, "Testing " << gain_stage << ": " << iter);
                const double ret_gain = test_radio->set_tx_gain(iter, gain_stage, chan);
                BOOST_CHECK_EQUAL(iter, ret_gain);
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_api_rx_gain_stage_test, x400_hbx_radio_fixture)
{
    const std::string log = "HBX_API_RX_GAIN_STAGE_TEST";

    for (const size_t chan : {0}) {
        test_radio->set_rx_gain_profile(HBX_GAIN_PROFILE_MANUAL, chan);

        UHD_LOG_INFO(
            log, "BEGIN TEST: rx" << chan << " GAIN STAGE CHANGE (SET->RETURN)\n");
        for (const auto& gain_stage : HBX_RX_GAIN_STAGES_MANUAL) {
            const auto gain_range   = test_radio->get_rx_gain_range(gain_stage, chan);
            const double prime_gain = (gain_range.start() == gain_range.stop())
                                          ? gain_range.start()
                                          : gain_range.stop();
            test_radio->set_rx_gain(prime_gain, gain_stage, chan);
            if (gain_range.step() == 0.0) {
                const double ret_gain =
                    test_radio->set_rx_gain(gain_range.start(), gain_stage, chan);
                BOOST_CHECK_EQUAL(gain_range.start(), ret_gain);
                continue;
            }
            for (double iter = gain_range.start(); iter <= gain_range.stop();
                 iter += gain_range.step()) {
                UHD_LOG_INFO(log, "Testing " << gain_stage << ": " << iter);
                const double ret_gain = test_radio->set_rx_gain(iter, gain_stage, chan);
                BOOST_CHECK_EQUAL(iter, ret_gain);
            }
        }
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_tx_gain_test, x400_hbx_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "HBX_TX_GAIN_TEST";
    const auto gain_range = test_radio->get_tx_gain_range(0);
    uhd::gain_range_t hbx_gain(gain_range.start(), gain_range.stop(), gain_range.step());

    for (const auto& fe_path : {fs_path("dboard/tx_frontends/0")}) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " GAIN CHANGE\n");
        for (double iter = hbx_gain.start(); iter <= hbx_gain.stop();
             iter += hbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);
            const auto gain_path = fe_path / "gains" / HBX_GAIN_STAGE_ALL / "value";
            tree->access<double>(gain_path).set(iter);
            const double ret_gain = tree->access<double>(gain_path).get();
            BOOST_CHECK_EQUAL(iter, ret_gain);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_rx_gain_test, x400_hbx_radio_fixture)
{
    auto tree             = test_radio->get_tree();
    const std::string log = "HBX_RX_GAIN_TEST";
    const auto gain_range = test_radio->get_rx_gain_range(0);
    uhd::gain_range_t hbx_gain(gain_range.start(), gain_range.stop(), gain_range.step());

    for (const auto& fe_path : {fs_path("dboard/rx_frontends/0")}) {
        UHD_LOG_INFO(log, "BEGIN TEST: " << fe_path << " GAIN CHANGE\n");
        for (double iter = hbx_gain.start(); iter <= hbx_gain.stop();
             iter += hbx_gain.step()) {
            UHD_LOG_INFO(log, "Testing gain: " << iter);
            const auto gain_path = fe_path / "gains" / HBX_GAIN_STAGE_ALL / "value";
            tree->access<double>(gain_path).set(iter);
            const double ret_gain = tree->access<double>(gain_path).get();
            BOOST_CHECK_EQUAL(iter, ret_gain);
        }
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_rx_antenna_test, x400_hbx_radio_fixture)
{
    const std::string log = "HBX_RX_ANTENNA_TEST";
    const size_t chan     = 0; // HBX has only 1 channel

    UHD_LOG_INFO(log, "BEGIN TEST: get available RX antennas\n");
    auto available_antennas = test_radio->get_rx_antennas(chan);
    UHD_LOG_INFO(log, "Available RX antennas: ");
    for (const auto& ant : available_antennas) {
        UHD_LOG_INFO(log, "  " << ant);
    }

    // Test that setting and getting the antennas works
    for (const std::string& ant : available_antennas) {
        UHD_LOG_INFO(log, "Testing antenna: " << ant);
        test_radio->set_rx_antenna(ant, chan);
        BOOST_REQUIRE_EQUAL(ant, test_radio->get_rx_antenna(chan));
    }
}

BOOST_FIXTURE_TEST_CASE(hbx_tx_antenna_test, x400_hbx_radio_fixture)
{
    const std::string log = "HBX_TX_ANTENNA_TEST";
    const size_t chan     = 0; // HBX has only 1 channel

    UHD_LOG_INFO(log, "BEGIN TEST: get available TX antennas\n");
    auto available_antennas = test_radio->get_tx_antennas(chan);
    UHD_LOG_INFO(log, "Available TX antennas: ");
    for (const auto& ant : available_antennas) {
        UHD_LOG_INFO(log, "  " << ant);
    }

    auto prev_ant = test_radio->get_tx_antenna(chan);
    for (const std::string& ant : available_antennas) {
        UHD_LOG_INFO(log, "Testing antenna: " << ant);
        test_radio->set_tx_antenna(ant, chan);
        BOOST_REQUIRE_EQUAL(ant, test_radio->get_tx_antenna(chan));
        prev_ant = ant;
    }
}

/******************************************************************************
 * RFNoC Graph Test
 *
 * This test case ensures that the Radio Block can be added to an RFNoC graph.
 *****************************************************************************/
BOOST_FIXTURE_TEST_CASE(hbx_radio_test_graph, x400_hbx_radio_fixture)
{
    detail::graph_t graph{};
    detail::graph_t::graph_edge_t edge_port_info0;
    edge_port_info0.src_port = 0;
    edge_port_info0.dst_port = 0;
    edge_port_info0.edge     = detail::graph_t::graph_edge_t::DYNAMIC;

    mock_radio_node_t mock_radio_block{0};
    mock_terminator_t mock_sink_term(1, {}, "MOCK_SINK");
    mock_terminator_t mock_source_term(1, {}, "MOCK_SOURCE");

    UHD_LOG_INFO("TEST", "Priming mock block properties");
    node_accessor.init_props(&mock_radio_block);
    mock_source_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::OUTPUT_EDGE, 0});
    mock_sink_term.set_edge_property<std::string>(
        "type", "sc16", {res_source_info::INPUT_EDGE, 0});

    UHD_LOG_INFO("TEST", "Creating graph...");
    graph.connect(&mock_source_term, test_radio.get(), edge_port_info0);
    graph.connect(test_radio.get(), &mock_sink_term, edge_port_info0);
    UHD_LOG_INFO("TEST", "Committing graph...");
    graph.commit();
    UHD_LOG_INFO("TEST", "Commit complete.");
}

// Helper lambdas that replicate the encoding math from hbx_iq_dc_correction_expert.
// coeff_to_fixed: convert a double coefficient to the COEFFS_FRAC_BITS fixed-point
// representation used by the RF core (COEFF_WIDTH bits wide).
// iq_to_dc_offset: pack I/Q DC offset into a single 32-bit register value.
// Both lambdas match the formulas in hbx_expert.cpp exactly.
//
// Note: _rfdc_dc_conv_offset is {0,0} for TX and {0,0} for RX in the mock
// (get_cal_coefs returns a single zero, giving i_avg=q_avg=0.0).
namespace {
constexpr auto coeff_to_fixed = [](double c) -> uint32_t {
    return static_cast<uint32_t>(c * (1u << COEFFS_FRAC_BITS))
           & ((1u << COEFF_WIDTH) - 1u);
};
constexpr auto iq_to_dc_offset = [](std::complex<double> dc) -> uint32_t {
    // rfdc_dc_conv_offset is {0,0} in the mock, so no subtraction needed.
    const int32_t i_s = static_cast<int32_t>(std::lrint(dc.real() * (1 << 15)));
    const int32_t q_s = static_cast<int32_t>(std::lrint(dc.imag() * (1 << 15)));
    return (static_cast<uint32_t>(static_cast<uint16_t>(q_s)) << 16)
           | static_cast<uint32_t>(static_cast<uint16_t>(i_s));
};
} // namespace

BOOST_FIXTURE_TEST_CASE(hbx_tx_iq_dc_compensation_test, x400_hbx_radio_fixture)
{
    // Prime the coerced TX frequency to above HBX_SWITCH_FREQ so the expert
    // uses the user-supplied coefficients instead of IQ_DC_DEFAULT_VALUES.
    test_radio->set_tx_frequency(8e9, 0);

    // Coefficients chosen so all expected register values are exact powers of 2,
    // making the assertions easy to reason about.
    uhd::iq_dc_cal_coeffs_t test_coeffs;
    test_coeffs.scaling_coeff = 0.5; // iinline = 0x400000
    test_coeffs.coeffs        = {{0.25, 0.125}}; // icross = 0x200000, qinline = 0x100000
    test_coeffs.group_delay   = 4.0; // group_delay register = 4
    test_coeffs.dc_offset     = {0.0, 0.0}; // DC register = 0

    // Writing the coefficients into the property tree immediately fires the
    // IQ/DC correction expert (AUTO_RESOLVE_ON_WRITE), which encodes them and
    // pokes the RF core registers.
    test_radio->get_tree()
        ->access<uhd::iq_dc_cal_coeffs_t>("dboard/tx_frontends/0/iq_balance/coeffs/value")
        .set(test_coeffs);

    // Verify that each register received the correctly encoded value.
    // The loop writes coefficients in reverse index order, so idx=0 is written last;
    // write_log therefore holds the encoding of coeffs[0] for ICROSS and QINLINE.
    const uint32_t iq_base = radio_control_impl::regmap::PERIPH_BASE + RF_CORE_ADDR_OFFSET
                             + IQ_IMPAIRMENTS_TX_OFFSET;
    const uint32_t dc_base =
        radio_control_impl::regmap::PERIPH_BASE + RF_CORE_ADDR_OFFSET + DC_TX_OFFSET;
    const auto& wl = reg_iface->write_log;

    BOOST_CHECK_EQUAL(wl.at(iq_base + IINLINE_COEFF_REG_OFFSET),
        coeff_to_fixed(test_coeffs.scaling_coeff));
    BOOST_CHECK_EQUAL(wl.at(iq_base + ICROSS_COEFF_REG_OFFSET),
        coeff_to_fixed(test_coeffs.coeffs[0].real()));
    BOOST_CHECK_EQUAL(wl.at(iq_base + QINLINE_COEFF_REG_OFFSET),
        coeff_to_fixed(test_coeffs.coeffs[0].imag()));
    BOOST_CHECK_EQUAL(wl.at(iq_base + GROUP_DELAY_REG_OFFSET),
        static_cast<uint32_t>(test_coeffs.group_delay));
    BOOST_CHECK_EQUAL(
        wl.at(dc_base + DC_VALUE_OFFSET), iq_to_dc_offset(test_coeffs.dc_offset));
}

BOOST_FIXTURE_TEST_CASE(hbx_rx_iq_dc_compensation_test, x400_hbx_radio_fixture)
{
    test_radio->set_rx_frequency(8e9, 0);

    uhd::iq_dc_cal_coeffs_t test_coeffs;
    test_coeffs.scaling_coeff = 0.5;
    test_coeffs.coeffs        = {{0.25, 0.125}};
    test_coeffs.group_delay   = 4.0;
    test_coeffs.dc_offset     = {0.0, 0.0};

    test_radio->get_tree()
        ->access<uhd::iq_dc_cal_coeffs_t>("dboard/rx_frontends/0/iq_balance/coeffs/value")
        .set(test_coeffs);

    const uint32_t iq_base = radio_control_impl::regmap::PERIPH_BASE + RF_CORE_ADDR_OFFSET
                             + IQ_IMPAIRMENTS_RX_OFFSET;
    const uint32_t dc_base =
        radio_control_impl::regmap::PERIPH_BASE + RF_CORE_ADDR_OFFSET + DC_RX_OFFSET;
    const auto& wl = reg_iface->write_log;

    BOOST_CHECK_EQUAL(wl.at(iq_base + IINLINE_COEFF_REG_OFFSET),
        coeff_to_fixed(test_coeffs.scaling_coeff));
    BOOST_CHECK_EQUAL(wl.at(iq_base + ICROSS_COEFF_REG_OFFSET),
        coeff_to_fixed(test_coeffs.coeffs[0].real()));
    BOOST_CHECK_EQUAL(wl.at(iq_base + QINLINE_COEFF_REG_OFFSET),
        coeff_to_fixed(test_coeffs.coeffs[0].imag()));
    BOOST_CHECK_EQUAL(wl.at(iq_base + GROUP_DELAY_REG_OFFSET),
        static_cast<uint32_t>(test_coeffs.group_delay));
    BOOST_CHECK_EQUAL(
        wl.at(dc_base + DC_VALUE_OFFSET), iq_to_dc_offset(test_coeffs.dc_offset));
}

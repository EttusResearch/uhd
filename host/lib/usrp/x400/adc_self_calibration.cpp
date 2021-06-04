//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "adc_self_calibration.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/utils/scope_exit.hpp>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace uhd { namespace features {

adc_self_calibration::adc_self_calibration(uhd::usrp::x400_rpc_iface::sptr rpcc,
    const std::string rpc_prefix,
    const std::string unique_id,
    size_t db_number,
    uhd::usrp::x400::x400_dboard_iface::sptr daughterboard)
    : _rpcc(rpcc)
    , _rpc_prefix(rpc_prefix)
    , _db_number(db_number)
    , _daughterboard(daughterboard)
    , _unique_id(unique_id)
{
}

void adc_self_calibration::run(size_t chan)
{
    const auto tx_gain_profile =
        _daughterboard->get_tx_gain_profile_api()->get_gain_profile(chan);
    const auto rx_gain_profile =
        _daughterboard->get_rx_gain_profile_api()->get_gain_profile(chan);
    if (tx_gain_profile != "default" || rx_gain_profile != "default") {
        throw uhd::runtime_error("Cannot run ADC self calibration when gain "
                                 "profile for RX or TX is not 'default'.");
    }

    // The frequency that we need to feed into the ADC is, by decree,
    // 13109 / 32768 times the ADC sample rate. (approx. 1.2GHz for a 3Gsps rate)
    const double spll_freq     = _rpcc->get_spll_freq();
    const double cal_tone_freq = spll_freq * 13109.0 / 32768.0;

    const auto cal_params = _daughterboard->get_adc_self_cal_params(cal_tone_freq);

    // Switch to CAL_LOOPBACK and save the current antenna
    const auto rx_antenna = _daughterboard->get_rx_antenna(chan);
    const auto tx_antenna = _daughterboard->get_tx_antenna(chan);

    auto reset_antennas = uhd::utils::scope_exit::make([&]() {
        _daughterboard->set_rx_antenna(rx_antenna, chan);
        _daughterboard->set_tx_antenna(tx_antenna, chan);

        // Waiting here allows some CPLD registers to be set. It's not clear
        // to me why we require this wait. See azdo #1473824
        constexpr auto fudge_time = 100ms;
        std::this_thread::sleep_for(fudge_time);
    });

    _daughterboard->set_rx_antenna("CAL_LOOPBACK", chan);
    _daughterboard->set_tx_antenna("CAL_LOOPBACK", chan);

    // Configure the output DAC mux to output 1/2 full scale
    // set_dac_mux_data uses 16-bit values.
    _rpcc->set_dac_mux_data(32768 / 2, 0);

    const size_t motherboard_channel_number = _db_number * 2 + chan;
    _rpcc->set_dac_mux_enable(motherboard_channel_number, 1);
    auto disable_dac_mux = uhd::utils::scope_exit::make(
        [&]() { _rpcc->set_dac_mux_enable(motherboard_channel_number, 0); });

    // Save all of the LO frequencies & sources
    const double original_rx_freq = _daughterboard->get_rx_frequency(chan);
    std::map<std::string, std::tuple<std::string, double>> rx_lo_state;
    for (auto rx_lo : _daughterboard->get_rx_lo_names(chan)) {
        const std::string source(_daughterboard->get_rx_lo_source(rx_lo, chan));
        const double freq = _daughterboard->get_rx_lo_freq(rx_lo, chan);
        rx_lo_state.emplace(std::piecewise_construct,
            std::forward_as_tuple(rx_lo),
            std::forward_as_tuple(source, freq));
    }
    auto restore_rx_los = uhd::utils::scope_exit::make([&]() {
        _daughterboard->set_rx_frequency(original_rx_freq, chan);
        for (auto entry : rx_lo_state) {
            auto& lo    = std::get<0>(entry);
            auto& state = std::get<1>(entry);

            auto& source      = std::get<0>(state);
            const double freq = std::get<1>(state);
            _daughterboard->set_rx_lo_source(source, lo, chan);
            _daughterboard->set_rx_lo_freq(freq, lo, chan);
        }
    });

    const double original_tx_freq = _daughterboard->get_tx_frequency(chan);
    std::map<std::string, std::tuple<std::string, double>> tx_lo_state;
    for (auto tx_lo : _daughterboard->get_tx_lo_names(chan)) {
        const std::string source(_daughterboard->get_tx_lo_source(tx_lo, chan));
        const double freq = _daughterboard->get_tx_lo_freq(tx_lo, chan);
        tx_lo_state.emplace(std::piecewise_construct,
            std::forward_as_tuple(tx_lo),
            std::forward_as_tuple(source, freq));
    }
    auto restore_tx_los = uhd::utils::scope_exit::make([&]() {
        _daughterboard->set_tx_frequency(original_tx_freq, chan);
        for (auto entry : tx_lo_state) {
            auto& lo    = std::get<0>(entry);
            auto& state = std::get<1>(entry);

            auto& source      = std::get<0>(state);
            const double freq = std::get<1>(state);
            _daughterboard->set_tx_lo_source(source, lo, chan);
            _daughterboard->set_tx_lo_freq(freq, lo, chan);
        }
    });

    _daughterboard->set_tx_frequency(cal_params.tx_freq, chan);
    _daughterboard->set_rx_frequency(cal_params.rx_freq, chan);

    // Set & restore the gain
    const double tx_gain = _daughterboard->get_tx_gain(chan);
    const double rx_gain = _daughterboard->get_rx_gain(chan);
    auto restore_gains   = uhd::utils::scope_exit::make([&]() {
        _daughterboard->get_rx_gain_profile_api()->set_gain_profile("default", chan);
        _daughterboard->get_tx_gain_profile_api()->set_gain_profile("default", chan);

        _daughterboard->set_tx_gain(tx_gain, chan);
        _daughterboard->set_rx_gain(rx_gain, chan);
    });

    // Set the threshold to detect half-scale
    // The setup_threshold call uses 14-bit ADC values
    constexpr int hysteresis_reset_time      = 100;
    constexpr int hysteresis_reset_threshold = 8000;
    constexpr int hysteresis_set_threshold   = 8192;
    _rpcc->setup_threshold(_db_number,
        chan,
        0,
        "hysteresis",
        hysteresis_reset_time,
        hysteresis_reset_threshold,
        hysteresis_set_threshold);
    bool found_gain = false;
    for (double i = cal_params.min_gain; i <= cal_params.max_gain; i += 1.0) {
        _daughterboard->get_rx_gain_profile_api()->set_gain_profile("default", chan);
        _daughterboard->get_tx_gain_profile_api()->set_gain_profile("default", chan);

        _daughterboard->set_tx_gain(i, chan);
        _daughterboard->set_rx_gain(i, chan);

        // Set the daughterboard to use the duplex entry in the DSA table which was
        // configured in the set_?x_gain call. (note that with a LabVIEW FPGA, we don't
        // control the ATR lines, hence why we override them here)
        _daughterboard->get_rx_gain_profile_api()->set_gain_profile("table_noatr", chan);
        _daughterboard->get_tx_gain_profile_api()->set_gain_profile("table_noatr", chan);

        _daughterboard->set_rx_gain(0b11, chan);
        _daughterboard->set_tx_gain(0b11, chan);

        // Wait for it to settle
        constexpr auto settle_time = 10ms;
        std::this_thread::sleep_for(settle_time);

        try {
            const bool threshold_status =
                _rpcc->get_threshold_status(_db_number, chan, 0);
            if (threshold_status) {
                found_gain = true;
                break;
            }
        } catch (uhd::runtime_error&) {
            // Catch & eat this error, the user has a 5.0 FPGA and so can't auto-gain
            return;
        }
    }

    if (!found_gain) {
        throw uhd::runtime_error(
            "Could not find appropriate gain for performing ADC self cal");
    }

    // (if required) unfreeze calibration
    const std::vector<int> current_frozen_state = _rpcc->get_cal_frozen(_db_number, chan);
    _rpcc->set_cal_frozen(0, _db_number, chan);
    auto refreeze_adcs = uhd::utils::scope_exit::make(
        [&]() { _rpcc->set_cal_frozen(current_frozen_state[0], _db_number, chan); });

    // Let the ADC calibrate
    // 2000ms was found experimentally to be sufficient
    constexpr auto calibration_time = 2000ms;
    std::this_thread::sleep_for(calibration_time);
}

}} // namespace uhd::features

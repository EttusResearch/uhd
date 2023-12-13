//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "adc_self_calibration.hpp"
#include <uhd/utils/cast.hpp>
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

void adc_self_calibration::run(size_t chan, uhd::device_addr_t params)
{
    const auto tx_gain_profile =
        _daughterboard->get_tx_gain_profile_api()->get_gain_profile(chan);
    const auto rx_gain_profile =
        _daughterboard->get_rx_gain_profile_api()->get_gain_profile(chan);
    if (tx_gain_profile != "default" || rx_gain_profile != "default") {
        throw uhd::runtime_error("Cannot run ADC self calibration when gain "
                                 "profile for RX or TX is not 'default'.");
    }

    // The frequency that we need to feed into the ADC is, by decree and if not specified
    // differently by the user, 13109 / 32768 times the ADC sample rate for X410.
    // (approx. 1.2GHz for a 3Gsps rate) This is at 0.4 * Fs, right on the boundary
    // between modes 1 and 2 for the ADC self-cal. But devices may also specify different
    // values in their ADC self cal params.
    const double cal_tone_freq = _daughterboard->get_converter_rate() * 13109.0 / 32768.0;

    auto cal_params = _daughterboard->get_adc_self_cal_params(cal_tone_freq);

    if (params.has_key("cal_freq")) {
        const double cal_freq = params.cast<double>("cal_freq", cal_params.rx_freq);
        // According to PG269 calibration modes are available for up to Fs / 2
        if (cal_freq < 0 or cal_freq > _daughterboard->get_converter_rate() / 2) {
            RFNOC_LOG_WARNING("Invalid value for `cal_freq`: "
                              << cal_freq
                              << ". Valid values are "
                                 "between 0 Hz and converter_rate / 2. Using default "
                              << cal_params.rx_freq / 1e6 << " MHz.");
        } else {
            cal_params.rx_freq = cal_freq;
            cal_params.tx_freq = cal_freq;
            // 0.4 * Fs is the limit between cal mode 2 and 1 according to PG269
            if (cal_freq <= (0.4 * _daughterboard->get_converter_rate())) {
                cal_params.calibration_mode = "calib_mode2";
            } else {
                cal_params.calibration_mode = "calib_mode1";
            }
            RFNOC_LOG_DEBUG("Custom cal_freq: " << cal_freq << " => Calibration Mode: "
                                                << cal_params.calibration_mode);
        }
    }
    if (params.has_key("cal_dac_mux_i")) {
        const int32_t dac_mux_i =
            uhd::cast::fromstr_cast<int32_t>(params.get("cal_dac_mux_i"));
        if (dac_mux_i < 0 or dac_mux_i > 0xFFFF) {
            RFNOC_LOG_WARNING("Invalid value for `cal_dac_mux_i`: "
                              << dac_mux_i << ". Using default value "
                              << cal_params.dac_iq_values.real());
        } else {
            cal_params.dac_iq_values.real(dac_mux_i);
            RFNOC_LOG_DEBUG("Custom cal_dac_mux_i: (" << dac_mux_i);
        }
    }
    if (params.has_key("cal_dac_mux_q")) {
        const int32_t dac_mux_q =
            uhd::cast::fromstr_cast<int32_t>(params.get("cal_dac_mux_q"));
        if (dac_mux_q < 0 or dac_mux_q > 0xFFFF) {
            RFNOC_LOG_WARNING("Invalid value for `cal_dac_mux_q`: "
                              << dac_mux_q << ". Using default value "
                              << cal_params.dac_iq_values.imag());
        } else {
            cal_params.dac_iq_values.imag(dac_mux_q);
            RFNOC_LOG_DEBUG("Custom cal_dac_mux_q: (" << dac_mux_q);
        }
    }
    if (params.has_key("cal_tone_duration")) {
        const uint32_t cal_time =
            params.cast<uint32_t>("cal_tone_duration", cal_params.calibration_time);
        if (cal_time > MAX_CAL_DURATION) {
            RFNOC_LOG_WARNING("Invalid value for `cal_tone_duration`: "
                              << cal_time << ". Maximum value: " << MAX_CAL_DURATION
                              << ". Using default " << cal_params.calibration_time
                              << " ms.");
        } else {
            cal_params.calibration_time = cal_time;
            RFNOC_LOG_DEBUG("Custom cal_tone_duration: " << cal_time << " ms");
        }
    }
    if (params.has_key("cal_delay")) {
        const uint32_t threshold_delay =
            params.cast<uint32_t>("cal_delay", cal_params.threshold_delay);
        cal_params.threshold_delay = threshold_delay;
        RFNOC_LOG_DEBUG("Custom cal_delay: " << cal_params.threshold_delay << " ms");
    }

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

    _rpcc->set_dac_mux_data(
        cal_params.dac_iq_values.real(), cal_params.dac_iq_values.imag());

    _rpcc->set_dac_mux_enable(_db_number, chan, 1);
    auto disable_dac_mux = uhd::utils::scope_exit::make(
        [&]() { _rpcc->set_dac_mux_enable(_db_number, chan, 0); });

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

    _rpcc->setup_threshold(_db_number,
        chan,
        0,
        "hysteresis",
        cal_params.threshold_delay,
        cal_params.threshold_under,
        cal_params.threshold_over);

    bool found_gain = _daughterboard->select_adc_self_cal_gain(chan);
    if (!found_gain) {
        throw uhd::runtime_error(
            "Could not find appropriate gain for performing ADC self cal");
    }

    _rpcc->set_calibration_mode(_db_number, chan, cal_params.calibration_mode);

    // (if required) unfreeze calibration
    const std::vector<int> current_frozen_state = _rpcc->get_cal_frozen(_db_number, chan);
    _rpcc->set_cal_frozen(0, _db_number, chan);
    auto refreeze_adcs = uhd::utils::scope_exit::make(
        [&]() { _rpcc->set_cal_frozen(current_frozen_state[0], _db_number, chan); });

    // Let the ADC calibrate
    // 2000ms was found experimentally to be sufficient
    auto calibration_time = std::chrono::milliseconds(cal_params.calibration_time);
    std::this_thread::sleep_for(calibration_time);
}

void adc_self_calibration::run(size_t chan)
{
    const uhd::device_addr_t params;
    adc_self_calibration::run(chan, params);
}

}} // namespace uhd::features

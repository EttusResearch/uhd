//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_experts.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/common/adrv9032_manager.hpp>
#include <cmath>

namespace {
constexpr double MIN_BLACKOUT_FREQ_125M = 451e6;
constexpr double MAX_BLACKOUT_FREQ_125M = 7.124e9;
} // namespace

namespace uhd { namespace usrp { namespace b300 {

void b300_desired_channel_frequency_expert::resolve()
{
    double coerced_freq = adrv9032_freq_range.clip(_freq_desired, true);
    if (coerced_freq > adrv9032_lo_freq_range.stop()) {
        if (uhd::math::frequencies_are_equal(_master_clock_rate, 125e6)) {
            // The maximum RFLO frequency cannot be used with the 125MHz master clock rate
            // due to needing to avoid blackout frequencies.
            _rflo_freq_desired = MAX_BLACKOUT_FREQ_125M;
            _nco_freq_desired  = coerced_freq - MAX_BLACKOUT_FREQ_125M;
        } else {
            _rflo_freq_desired = adrv9032_lo_freq_range.stop();
            _nco_freq_desired  = coerced_freq - adrv9032_lo_freq_range.stop();
        }
    } else if (coerced_freq < adrv9032_lo_freq_range.start()) {
        if (uhd::math::frequencies_are_equal(_master_clock_rate, 125e6)) {
            // The minimum RFLO frequency cannot be used with the 125MHz master clock rate
            // due to needing to avoid blackout frequencies.
            _rflo_freq_desired = MIN_BLACKOUT_FREQ_125M;
            _nco_freq_desired  = coerced_freq - MIN_BLACKOUT_FREQ_125M;
        } else {
            _rflo_freq_desired = adrv9032_lo_freq_range.start();
            _nco_freq_desired  = coerced_freq - adrv9032_lo_freq_range.start();
        }
    } else {
        double coerced_rflo_freq = avoid_blackout_frequencies(
            coerced_freq, _master_clock_rate, uhd::usrp::adrv9032_lo_freq_range);
        _rflo_freq_desired = coerced_rflo_freq;
        _nco_freq_desired  = coerced_freq - coerced_rflo_freq;
    }
}

void b300_rflo_channel_frequency_expert::resolve()
{
    // Don't handle the change of LO source in this expert, the LO source change is
    // handled in the b300_rflo_programming_expert.
    if (!_rflo_source.is_dirty()) {
        if (!rflo_frequency_allowed(_rflo_freq_desired, _master_clock_rate)) {
            // Allow users to set blackout frequencies if truly desired. If the channel
            // frequency is being set like normal, this case will be avoided in the
            // b300_desired_channel_frequency_expert. But a user can specifically set the
            // RFLO frequency if they want to avoid the NCO.
            UHD_LOG_WARNING("B300",
                "Requested RFLO frequency is within 1MHz of a blackout frequency that "
                "can cause issues with the Tx QEC Tracking calibration. It is "
                "recommended to use a different RFLO frequency with NCO shift to "
                "achieve this desired frequency.");
        }
        if (_rflo_source == "LO0") {
            _rflo0_frequency = _rflo_freq_desired;
        } else {
            _rflo1_frequency = _rflo_freq_desired;
        }
        if (std::abs(_rflo0_frequency - _rflo1_frequency) < 50e6) {
            UHD_LOG_WARNING("B300",
                "It is not recommended for the ADRV9032 RFLOs to be set within 50 MHz "
                "of each other as it can cause unwanted coupling between the PLLs. If "
                "channels are desired to be at nearby frequencies, consider setting "
                "both channels to be sourced by the same LO.");
        }
    }
}

void b300_nco_programming_expert::resolve()
{
    // Calculation is done for Tx NCO, but then needs to be negated for Rx since it is
    // a shift. So when the requested frequency is higher than the LO max, Tx needs a
    // positive shift, but Rx needs a negative shift, and then vice versa for lower
    // than LO min.
    _nco_freq_coerced = _nco_freq_desired;
    auto freq_to_set  = _dir == TX_DIRECTION ? _nco_freq_desired : -_nco_freq_desired;
    _set_nco_frequency(freq_to_set);
}

void b300_rflo_programming_expert::resolve()
{
    // Currently the adrv9032 manager takes in the channel and direction and handles
    // setting the correct LO, we only need to program it once, so keep track of when it
    // gets set. But we still need to keep checking each channel's source to update the
    // coerced frequency outputs. If LO source changing is what triggers this expert, then
    // also skip programming the LO, since the function to change the LO source already
    // programs the LO frequencies.
    bool lo_programmed = _rflo_frequency.is_dirty() ? false : true;
    if (_rx0_rflo_source == _lo_name) {
        _rx0_rflo_freq_coerced = _rflo_frequency;
        if (!lo_programmed) {
            _set_rflo_frequency(RX_DIRECTION, 0, _rflo_frequency);
            lo_programmed = true;
        }
    }
    if (_rx1_rflo_source == _lo_name) {
        _rx1_rflo_freq_coerced = _rflo_frequency;
        if (!lo_programmed) {
            _set_rflo_frequency(RX_DIRECTION, 1, _rflo_frequency);
            lo_programmed = true;
        }
    }
    if (_tx0_rflo_source == _lo_name) {
        _tx0_rflo_freq_coerced = _rflo_frequency;
        if (!lo_programmed) {
            _set_rflo_frequency(TX_DIRECTION, 0, _rflo_frequency);
            lo_programmed = true;
        }
    }
    if (_tx1_rflo_source == _lo_name) {
        _tx1_rflo_freq_coerced = _rflo_frequency;
        if (!lo_programmed) {
            _set_rflo_frequency(TX_DIRECTION, 1, _rflo_frequency);
            lo_programmed = true;
        }
    }
}

void b300_coerced_channel_frequency_expert::resolve()
{
    _freq_coerced = _rflo_freq_coerced + _nco_freq_coerced;
}

}}} // namespace uhd::usrp::b300

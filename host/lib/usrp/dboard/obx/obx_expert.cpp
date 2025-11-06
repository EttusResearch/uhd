//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "obx_expert.hpp"
#include "../db_obx.hpp"
#include "obx_cpld_ctrl.hpp"
#include "obx_gpio_ctrl.hpp"
#include <uhd/types/device_addr.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/usrp/fe_connection.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/max287x.hpp>
#include <boost/algorithm/string.hpp>

namespace uhd { namespace usrp { namespace dboard { namespace obx {

void _sync_phase(uhd::time_spec_t cmd_time,
    uhd::direction_t dir,
    dboard_iface::sptr db_iface,
    obx_gpio_ctrl::sptr gpio_ctrl,
    int64_t sync_delay)
{
    // Send phase sync signal only if the command time is set
    if (cmd_time != uhd::time_spec_t(0.0)) {
        // Delay 400 microseconds to allow LOs to lock
        cmd_time += uhd::time_spec_t(0.0004);

        // Phase synchronization for MAX2871 requires that the sync signal
        // is at least 4/(N*PFD_freq) + 2.6ns before the rising edge of the
        // ref clock and 4/(N*PFD_freq) after the rising edge of the ref clock.
        // Since the MAX2871 requires the ref freq and PFD freq be the same
        // for phase synchronization, the dboard clock rate is used as the PFD
        // freq and the sync signal is aligned to the falling edge to meet
        // the setup and hold requirements.  Since the command time ticks
        // at the radio clock rate, this only works if the radio clock is
        // an even multiple of the dboard clock, the dboard clock is a
        // multiple of the system reference, and the device time has been
        // set on a PPS edge sampled by the system reference clock.

        const double pfd_freq = db_iface->get_clock_rate(
            dir == TX_DIRECTION ? dboard_iface::UNIT_TX : dboard_iface::UNIT_RX);
        const double tick_rate = db_iface->get_codec_rate(
            dir == TX_DIRECTION ? dboard_iface::UNIT_TX : dboard_iface::UNIT_RX);
        const int64_t ticks_per_pfd_cycle = (int64_t)(tick_rate / pfd_freq);

        // Convert time to ticks
        int64_t ticks = cmd_time.to_ticks(tick_rate);
        // Align time to next falling edge of dboard clock
        ticks += ticks_per_pfd_cycle - (ticks % ticks_per_pfd_cycle)
                 + (ticks_per_pfd_cycle / 2);
        // Add any user specified delay
        ticks += sync_delay;
        // Set the command time
        cmd_time = uhd::time_spec_t::from_ticks(ticks, tick_rate);
        db_iface->set_command_time(cmd_time);

        // Assert SYNC
        obx_gpio_field_info_t lo1_field_info =
            gpio_ctrl->gpio_map.find(dir == TX_DIRECTION ? TXLO1_SYNC : RXLO1_SYNC)
                ->second;
        obx_gpio_field_info_t lo2_field_info =
            gpio_ctrl->gpio_map.find(dir == TX_DIRECTION ? TXLO2_SYNC : RXLO2_SYNC)
                ->second;
        uint16_t value = (1 << lo1_field_info.offset) | (1 << lo2_field_info.offset);
        uint16_t mask  = lo1_field_info.mask | lo2_field_info.mask;
        dboard_iface::unit_t unit = lo1_field_info.unit;
        UHD_ASSERT_THROW(lo1_field_info.unit == lo2_field_info.unit);
        db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_IDLE, value, mask);
        cmd_time += uhd::time_spec_t(1 / pfd_freq);
        db_iface->set_command_time(cmd_time);
        db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_TX_ONLY, value, mask);
        cmd_time += uhd::time_spec_t(1 / pfd_freq);
        db_iface->set_command_time(cmd_time);
        db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_RX_ONLY, value, mask);
        cmd_time += uhd::time_spec_t(1 / pfd_freq);
        db_iface->set_command_time(cmd_time);
        db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_FULL_DUPLEX, value, mask);

        // De-assert SYNC
        // Head of line blocking means the command time does not need to be set.
        db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_IDLE, 0, mask);
        db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_TX_ONLY, 0, mask);
        db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_RX_ONLY, 0, mask);
        db_iface->set_atr_reg(unit, gpio_atr::ATR_REG_FULL_DUPLEX, 0, mask);
    }
}

void obx_tx_frequency_expert::resolve()
{
    double freq_lo1 = 0.0;
    double freq_lo2 = 0.0;
    double ref_freq = _db_iface->get_clock_rate(dboard_iface::UNIT_TX);
    bool is_int_n   = false;

    /*
     * If the user sets 'mode_n=integer' in the tuning args, the user wishes to
     * tune in Integer-N mode, which can result in better spur
     * performance on some mixers. The default is fractional tuning.
     */

    device_addr_t tune_args = _tune_args.get();

    is_int_n = boost::iequals(tune_args.get("mode_n", ""), "integer");
    UHD_LOGGER_TRACE("OBX") << "OBX TX: the requested frequency is "
                            << (_freq_desired / 1e6) << " MHz";
    double target_pfd_freq = _target_pfd_freq;
    if (is_int_n and tune_args.has_key("int_n_step")) {
        target_pfd_freq = tune_args.cast<double>("int_n_step", _target_pfd_freq);
        if (target_pfd_freq > _target_pfd_freq) {
            UHD_LOGGER_WARNING("OBX")
                << "Requested int_n_step of " << (target_pfd_freq / 1e6)
                << " MHz too large, clipping to " << (_target_pfd_freq / 1e6) << " MHz";
            target_pfd_freq = _target_pfd_freq;
        }
    }

    // Clip the frequency to the valid range
    double freq = obx_freq_range.clip(_freq_desired);

    // Power up/down LOs
    if (_lo1->is_shutdown())
        _lo1->power_up();
    if (_lo2->is_shutdown() and (_power_mode == PERFORMANCE or freq < (500e6)))
        _lo2->power_up();
    else if (freq >= 500e6 and _power_mode == POWERSAVE)
        _lo2->shutdown();

    // Set up LOs for phase sync if command time is set
    uhd::time_spec_t cmd_time = _db_iface->get_command_time();
    if (cmd_time != uhd::time_spec_t(0.0)) {
        _lo1->config_for_sync(true);
        if (not _lo2->is_shutdown())
            _lo2->config_for_sync(true);
    } else {
        _lo1->config_for_sync(false);
        if (not _lo2->is_shutdown())
            _lo2->config_for_sync(false);
    }

    // Set up registers for the requested frequency
    if (freq < (500e6)) {
        _cpld->set_tx_path(LFCN_2250, LOW_BAND);
        // Set LO1 to IF of 2100 MHz (offset from RX IF to reduce leakage)
        freq_lo1 = _lo1->set_frequency(2100e6, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
        // Set LO2 to IF minus desired frequency
        freq_lo2 =
            _lo2->set_frequency(freq_lo1 - freq, ref_freq, target_pfd_freq, is_int_n);
        _lo2->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
    } else if (freq <= (800e6)) {
        _cpld->set_tx_path(LFCN_800, MID_BAND);
        freq_lo1 = _lo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
    } else if (freq <= (1000e6)) {
        _cpld->set_tx_path(LFCN_800, MID_BAND);
        freq_lo1 = _lo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
    } else if (freq <= (2200e6)) {
        _cpld->set_tx_path(LFCN_2250, MID_BAND);
        freq_lo1 = _lo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
    } else if (freq <= (2500e6)) {
        _cpld->set_tx_path(LFCN_2250, MID_BAND);
        freq_lo1 = _lo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
    } else if (freq <= (6000e6)) {
        _cpld->set_tx_path(NO_FILTER, MID_BAND);
        freq_lo1 = _lo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
    } else if (freq <= (8400e6)) {
        _cpld->set_tx_path(NO_FILTER, HIGH_BAND);
        freq_lo1 = _lo1->set_frequency(2600e6, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
        freq_lo2 = _lo2->set_frequency(0.5 * (freq_lo1 + freq),
            ref_freq,
            target_pfd_freq,
            is_int_n,
            max287x_iface::AUX_OUT);
        _lo2->set_aux_output_power(max287x_iface::AUX_OUTPUT_POWER_2DBM);
    }

    // To reduce the number of commands issued to the device, write to the
    // SPI destination already addressed first.  This avoids the writes to
    // the GPIO registers to route the SPI to the same destination.
    switch (_gpio->get_field(SPI_ADDR)) {
        case TXLO1:
            _lo1->commit();
            if (freq < (500e6) || freq > (6000e6))
                _lo2->commit();
            _cpld->write();
            break;
        case TXLO2:
            if (freq < (500e6) || freq > (6000e6))
                _lo2->commit();
            _lo1->commit();
            _cpld->write();
            break;
        default:
            _cpld->write();
            _lo1->commit();
            if (freq < (500e6) || freq > (6000e6))
                _lo2->commit();
            break;
    }

    if (cmd_time != uhd::time_spec_t(0.0) and _lo1->can_sync()) {
        _sync_phase(cmd_time, TX_DIRECTION, _db_iface, _gpio, _sync_delay);
    }

    _freq_coerced = freq <= 6000e6 ? freq_lo1 - freq_lo2 : 2 * freq_lo2 - freq_lo1;

    UHD_LOGGER_TRACE("OBX") << "OBX TX: the actual frequency is " << (_freq_coerced / 1e6)
                            << " MHz";
}

void obx_rx_frequency_expert::resolve()
{
    double freq_lo1 = 0.0;
    double freq_lo2 = 0.0;
    double ref_freq = _db_iface->get_clock_rate(dboard_iface::UNIT_RX);
    bool is_int_n   = false;

    UHD_LOGGER_TRACE("OBX") << "OBX RX: the requested frequency is "
                            << (_freq_desired / 1e6) << " MHz";

    device_addr_t tune_args = _tune_args.get();
    is_int_n                = boost::iequals(tune_args.get("mode_n", ""), "integer");
    double target_pfd_freq  = _target_pfd_freq;
    if (is_int_n and tune_args.has_key("int_n_step")) {
        target_pfd_freq = tune_args.cast<double>("int_n_step", _target_pfd_freq);
        if (target_pfd_freq > _target_pfd_freq) {
            UHD_LOGGER_WARNING("OBX")
                << "Requested int_n_step of " << (target_pfd_freq / 1e6)
                << " Mhz too large, clipping to " << (_target_pfd_freq / 1e6) << " MHz";
            target_pfd_freq = _target_pfd_freq;
        }
    }

    // Clip the frequency to the valid range
    double freq = obx_freq_range.clip(_freq_desired);

    // Power up/down LOs
    if (_lo1->is_shutdown())
        _lo1->power_up();
    if (_lo2->is_shutdown() and (_power_mode == PERFORMANCE or freq < 500e6))
        _lo2->power_up();
    else if (freq >= 500e6 and _power_mode == POWERSAVE)
        _lo2->shutdown();

    // Set up LOs for phase sync if command time is set
    uhd::time_spec_t cmd_time = _db_iface->get_command_time();
    if (cmd_time != uhd::time_spec_t(0.0)) {
        _lo1->config_for_sync(true);
        if (not _lo2->is_shutdown())
            _lo2->config_for_sync(true);
    } else {
        _lo1->config_for_sync(false);
        if (not _lo2->is_shutdown())
            _lo2->config_for_sync(false);
    }

    // Work with frequencies
    if (freq < 42e6) {
        _cpld->set_rx_path(NO_FILTER, LOW_BAND, SEL_LNA_MAAL);
        // The filter on the IF has a center of 2440 MHz and an 84 MHz bandwidth.
        // For frequencies under 42 MHz, the LO for the IF will be in the filter
        // bandwidth.  Shift the IF to move the LO to the edge of the filter bandwidth
        // to reduce LO leakage.
        double if_freq = 2398e6 + freq;
        freq_lo1 = _lo1->set_frequency(if_freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
        // Set LO2 to IF minus desired frequency
        freq_lo2 =
            _lo2->set_frequency(freq_lo1 - freq, ref_freq, target_pfd_freq, is_int_n);
        _lo2->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
    } else if (freq < 500e6) {
        _cpld->set_rx_path(NO_FILTER, LOW_BAND, SEL_LNA_MAAL);
        // Set LO1 to IF of 2440 (center of filter)
        freq_lo1 = _lo1->set_frequency(2440e6, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
        // Set LO2 to IF minus desired frequency
        freq_lo2 =
            _lo2->set_frequency(freq_lo1 - freq, ref_freq, target_pfd_freq, is_int_n);
        _lo2->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
    } else if (freq < 800e6) {
        _cpld->set_rx_path(LFCN_800, MID_BAND, SEL_LNA_MAAL);
        freq_lo1 = _lo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
    } else if (freq < 1000e6) {
        _cpld->set_rx_path(LFCN_800, MID_BAND, SEL_LNA_MAAL);
        freq_lo1 = _lo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
    } else if (freq < 1500e6) {
        _cpld->set_rx_path(LFCN_2250, MID_BAND, SEL_LNA_MAAL);
        freq_lo1 = _lo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
    } else if (freq < 2500e6) {
        _cpld->set_rx_path(LFCN_2250, MID_BAND, SEL_LNA_PMA3);
        freq_lo1 = _lo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_2DBM);
    } else if (freq <= 6000e6) {
        _cpld->set_rx_path(NO_FILTER, MID_BAND, SEL_LNA_PMA3);
        freq_lo1 = _lo1->set_frequency(freq, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
    } else if (freq <= (8400e6)) {
        _cpld->set_rx_path(NO_FILTER, HIGH_BAND, SEL_LNA_PMA3);
        freq_lo1 = _lo1->set_frequency(2600e6, ref_freq, target_pfd_freq, is_int_n);
        _lo1->set_output_power(max287x_iface::OUTPUT_POWER_5DBM);
        freq_lo2 = _lo2->set_frequency(0.5 * (freq_lo1 + freq),
            ref_freq,
            target_pfd_freq,
            is_int_n,
            max287x_iface::AUX_OUT);
        _lo2->set_aux_output_power(max287x_iface::AUX_OUTPUT_POWER_2DBM);
    }

    // To reduce the number of commands issued to the device, write to the
    // SPI destination already addressed first.  This avoids the writes to
    // the GPIO registers to route the SPI to the same destination.
    switch (_gpio->get_field(SPI_ADDR)) {
        case RXLO1:
            _lo1->commit();
            if (freq < (500e6) || freq > (6000e6))
                _lo2->commit();
            _cpld->write();
            break;
        case RXLO2:
            if (freq < (500e6) || freq > (6000e6))
                _lo2->commit();
            _lo1->commit();
            _cpld->write();
            break;
        default:
            _cpld->write();
            _lo1->commit();
            if (freq < (500e6) || freq > (6000e6))
                _lo2->commit();
            break;
    }

    if (cmd_time != uhd::time_spec_t(0.0) and _lo1->can_sync()) {
        _sync_phase(cmd_time, RX_DIRECTION, _db_iface, _gpio, _sync_delay);
    }

    _freq_coerced = freq <= 6000e6 ? freq_lo1 - freq_lo2 : 2 * freq_lo2 - freq_lo1;

    UHD_LOGGER_TRACE("OBX") << "OBX RX: the actual frequency is " << (_freq_coerced / 1e6)
                            << " MHz";
}

void obx_tx_frontend_expert::resolve()
{
    try {
        if (_freq_coerced <= 6000e6) {
            _db_iface->set_fe_connection(
                dboard_iface::UNIT_TX, "0", usrp::fe_connection_t("QI", 0.0));
        } else {
            _db_iface->set_fe_connection(
                dboard_iface::UNIT_TX, "0", usrp::fe_connection_t("IQ", 0.0));
        }
    } catch (const std::exception& e) {
        // During initialization, the board will set the frequency before the frontend
        // connection is added to the dboard_iface
        UHD_LOGGER_TRACE("OBX") << "Error setting FE connection: " << e.what();
    }
}

void obx_rx_frontend_expert::resolve()
{
    try {
        if (_freq_coerced <= 6000e6) {
            _db_iface->set_fe_connection(
                dboard_iface::UNIT_RX, "0", usrp::fe_connection_t("IQ", 0.0));
        } else {
            _db_iface->set_fe_connection(
                dboard_iface::UNIT_RX, "0", usrp::fe_connection_t("QI", 0.0));
        }
    } catch (const std::exception& e) {
        // During initialization, the board will set the frequency before the frontend
        // connection is added to the dboard_iface
        UHD_LOGGER_TRACE("OBX") << "Error setting FE connection: " << e.what();
    }
}

void obx_tx_antenna_expert::resolve()
{
    // validate input
    assert_has(obx_tx_antennas, _antenna.get(), "obx tx antenna name");
    _cpld->set_field(FE_SEL_CAL_TX2, (_antenna == "CAL"));
    _cpld->write();
}

void obx_rx_antenna_expert::resolve()
{
    // validate input
    assert_has(obx_rx_antennas, _antenna.get(), "obx rx antenna name");
    _gpio->set_field(RX2_EN_N, (_antenna != "RX2"));
    _gpio->write();
    // This value doesn't make sense, but is how the schematic is wired/labeled
    _cpld->set_field(FE_SEL_CAL_RX2, (_antenna != "CAL"));
    // There can be long transients on TX, so force on the TX PA
    // except when in powersave mode.
    _cpld->set_field(TXDRV_FORCEON, (_power_mode == POWERSAVE ? 0 : 1));
    _cpld->write();
}

int _get_gain_setting(double gain)
{
    // OBX gain is controlled by programming a digital step attenuator. The DSA increases
    // attenuation as the programmed value is increased, so when the user wants 0 gain, we
    // want to program the highest attenutation, and then decrease the attenuation as the
    // user increases gain, until we eventually reach 0 attenuation when the user requests
    // maximum gain. We accomplish this by taking the maximum gain subtracted by the user
    // requested gain.
    //
    // The DSA being used provides 7 pins to control the attenuation, however the pin that
    // controls 0.25 dB of attenuation is hardwired to 0, so we only set 6 of the pins
    // that control the attenuation. Each pin represents a different level of attenuation
    // Per pin: 16, 8, 4, 2, 1, and 0.5 dB of attenuation.
    int attn_code = (obx_gain_range.stop() * 2) - int(std::floor(gain * 2));
    return attn_code;
}

void obx_tx_gain_expert::resolve()
{
    _coerced_gain       = obx_gain_range.clip(_desired_gain);
    const int attn_code = _get_gain_setting(_coerced_gain);
    _gpio->set_field(TX_GAIN, attn_code);
    _gpio->write();
    UHD_LOG_TRACE("OBX",
        "OBX TX Gain: " << _coerced_gain.get() << " dB, Code: " << attn_code
                        << ", IO Bits " << (attn_code << 10));
}

void obx_rx_gain_expert::resolve()
{
    _coerced_gain       = obx_gain_range.clip(_desired_gain);
    const int attn_code = _get_gain_setting(_coerced_gain);
    _gpio->set_field(RX_GAIN, attn_code);
    _gpio->write();
    UHD_LOG_TRACE("OBX",
        "OBX RX Gain: " << _coerced_gain.get() << " dB, Code: " << attn_code
                        << ", IO Bits " << (attn_code << 10));
}

void obx_xcvr_mode_expert::resolve()
{
    // The intent is to add behavior based on whether
    // the board is in TX, RX, or full duplex mode
    // to reduce power consumption and RF noise.
    std::string mode = _desired_xcvr_mode.get();
    boost::to_upper(mode);
    if (mode == "FDX") {
        _coerced_xcvr_mode = FDX;
    } else if (mode == "TDD") {
        _coerced_xcvr_mode = TDD;
        _cpld->set_field(TXDRV_FORCEON, 1);
        _cpld->write();
    } else if (mode == "TX") {
        _coerced_xcvr_mode = TX;
    } else if (mode == "RX") {
        _coerced_xcvr_mode = RX;
    } else {
        throw uhd::value_error("invalid xcvr_mode");
    }
}

void obx_temp_comp_mode_expert::resolve()
{
    const bool enabled = [this]() {
        if (_temp_comp_mode == "enabled") {
            return true;
        } else if (_temp_comp_mode == "disabled") {
            return false;
        } else {
            throw uhd::value_error("invalid temperature_compensation_mode");
        }
    }();

    for (const auto& lo : {_txlo1, _txlo2, _rxlo1, _rxlo2}) {
        lo->set_auto_retune(enabled);
    }
}

void obx_power_mode_expert::resolve()
{
    if (_desired_power_mode == "performance") {
        // performance mode attempts to reduce tuning and settling time
        // as much as possible without adding noise.

        // RXLNA2 has a ~100ms warm up time, so the LNAs are forced on
        // here to reduce the settling time as much as possible.  The
        // force on signals are gated by the LNA selection so the LNAs
        // are turned on/off during tuning.  Unfortunately, that means
        // there is still a long settling time when tuning from the high
        // band (>1.5 GHz) to the low band (<1.5 GHz).
        _cpld->set_field(RXLNA1_FORCEON, 1);
        _cpld->set_field(RXLNA2_FORCEON, 1);

        // Some components need to be forced on to reduce settling time.
        // Note that some FORCEON lines are still gated by other bits
        // in the CPLD register and are asserted during frequency tuning.
        _cpld->set_field(RXAMP_FORCEON, 1);
        _cpld->set_field(RXDEMOD_FORCEON, 1);
        _cpld->set_field(RXDRV_FORCEON, 1);
        _cpld->set_field(RXMIXER_FORCEON, 0);
        _cpld->set_field(RXLO1_FORCEON, 1);
        _cpld->set_field(RXLO2_FORCEON, 1);
        _cpld->set_field(RXDOUBLER_FORCEON, 1);
        _cpld->write();

        _coerced_power_mode = PERFORMANCE;
    } else if (_desired_power_mode == "powersave") {
        // powersave mode attempts to use the least amount of power possible
        // by powering on components only when needed.  Longer tuning and
        // settling times are expected.

        // Clear the LNA force on bits.
        _cpld->set_field(RXLNA1_FORCEON, 0);
        _cpld->set_field(RXLNA2_FORCEON, 0);
        _cpld->write();

        _coerced_power_mode = POWERSAVE;
    }
}

}}}} // namespace uhd::usrp::dboard::obx

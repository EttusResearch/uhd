//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rhodium_radio_control.hpp"
#include "rhodium_constants.hpp"
#include <uhd/exception.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/common/apply_corrections.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <cmath>
#include <cstdlib>
#include <memory>
#include <sstream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::math::fp_compare;

namespace {
constexpr char RX_FE_CONNECTION_LOWBAND[]  = "QI";
constexpr char RX_FE_CONNECTION_HIGHBAND[] = "IQ";
constexpr char TX_FE_CONNECTION_LOWBAND[]  = "QI";
constexpr char TX_FE_CONNECTION_HIGHBAND[] = "IQ";

constexpr uint64_t SET_RATE_RPC_TIMEOUT_MS = 10000;

} // namespace


/******************************************************************************
 * Structors
 *****************************************************************************/
rhodium_radio_control_impl::rhodium_radio_control_impl(make_args_ptr make_args)
    : radio_control_impl(std::move(make_args))
{
    RFNOC_LOG_TRACE("Entering rhodium_radio_control_impl ctor...");
    UHD_ASSERT_THROW(get_block_id().get_block_count() < 2);
    const char radio_slot_name[] = {'A', 'B'};
    _radio_slot                  = radio_slot_name[get_block_id().get_block_count()];
    _rpc_prefix                  = (_radio_slot == "A") ? "db_0_" : "db_1_";
    RFNOC_LOG_TRACE("Radio slot: " << _radio_slot);
    UHD_ASSERT_THROW(get_num_input_ports() == RHODIUM_NUM_CHANS);
    UHD_ASSERT_THROW(get_num_output_ports() == RHODIUM_NUM_CHANS);
    UHD_ASSERT_THROW(get_mb_controller());
    _n320_mb_control = std::dynamic_pointer_cast<mpmd_mb_controller>(get_mb_controller());
    UHD_ASSERT_THROW(_n320_mb_control);
    _n3xx_timekeeper = std::dynamic_pointer_cast<mpmd_mb_controller::mpmd_timekeeper>(
        _n320_mb_control->get_timekeeper(0));
    UHD_ASSERT_THROW(_n3xx_timekeeper);
    _rpcc = _n320_mb_control->get_rpc_client();
    UHD_ASSERT_THROW(_rpcc);

    const auto all_dboard_info =
        _rpcc->request<std::vector<std::map<std::string, std::string>>>(
            "get_dboard_info");
    RFNOC_LOG_TRACE("Hardware detected " << all_dboard_info.size() << " daughterboards.");

    // If we two radio blocks, but there is only one dboard plugged in, we skip
    // initialization. The board needs to be in slot A
    if (all_dboard_info.size() > get_block_id().get_block_count()) {
        _init_defaults();
        _init_mpm();
        _init_peripherals();
        _init_prop_tree();
    }

    // Properties
    for (auto& samp_rate_prop : _samp_rate_in) {
        samp_rate_prop.set(_master_clock_rate);
    }
    for (auto& samp_rate_prop : _samp_rate_out) {
        samp_rate_prop.set(_master_clock_rate);
    }
}

rhodium_radio_control_impl::~rhodium_radio_control_impl()
{
    RFNOC_LOG_TRACE("rhodium_radio_control_impl::dtor() ");
}


/******************************************************************************
 * RF API Calls
 *****************************************************************************/
double rhodium_radio_control_impl::set_rate(double requested_rate)
{
    meta_range_t rates;
    for (const double rate : RHODIUM_RADIO_RATES) {
        rates.push_back(range_t(rate));
    }

    const double rate = rates.clip(requested_rate);
    if (!math::frequencies_are_equal(requested_rate, rate)) {
        RFNOC_LOG_WARNING("Coercing requested sample rate from "
                          << (requested_rate / 1e6) << " MHz to " << (rate / 1e6)
                          << " MHz, the closest possible rate.");
    }

    const double current_rate = get_rate();
    if (math::frequencies_are_equal(current_rate, rate)) {
        RFNOC_LOG_DEBUG(
            "Rate is already at " << (rate / 1e6) << " MHz. Skipping set_rate()");
        return current_rate;
    }

    RFNOC_LOG_TRACE("Updating master clock rate to " << rate);
    _master_clock_rate = _rpcc->request_with_token<double>(
        SET_RATE_RPC_TIMEOUT_MS, "db_0_set_master_clock_rate", rate);
    _n3xx_timekeeper->update_tick_rate(_master_clock_rate);
    radio_control_impl::set_rate(_master_clock_rate);
    // The lowband LO frequency will change with the master clock rate, so
    // update the tuning of the device.
    set_tx_frequency(get_tx_frequency(0), 0);
    set_rx_frequency(get_rx_frequency(0), 0);

    set_tick_rate(_master_clock_rate);
    return _master_clock_rate;
}

void rhodium_radio_control_impl::set_tx_antenna(const std::string& ant, const size_t chan)
{
    RFNOC_LOG_TRACE("set_tx_antenna(ant=" << ant << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    if (!uhd::has(RHODIUM_TX_ANTENNAS, ant)) {
        RFNOC_LOG_ERROR("Invalid TX antenna value: " << ant);
        throw uhd::value_error("Requesting invalid TX antenna value!");
    }

    _update_tx_output_switches(ant);
    // _update_atr will set the cached antenna value, so no need to do
    // it here. See comments in _update_antenna for more info.
    _update_atr(ant, TX_DIRECTION);
}

void rhodium_radio_control_impl::set_rx_antenna(const std::string& ant, const size_t chan)
{
    RFNOC_LOG_TRACE("Setting RX antenna to " << ant);
    UHD_ASSERT_THROW(chan == 0);

    if (!uhd::has(RHODIUM_RX_ANTENNAS, ant)) {
        RFNOC_LOG_ERROR("Invalid RX antenna value: " << ant);
        throw uhd::value_error("Requesting invalid RX antenna value!");
    }

    _update_rx_input_switches(ant);
    // _update_atr will set the cached antenna value, so no need to do
    // it here. See comments in _update_antenna for more info.
    _update_atr(ant, RX_DIRECTION);
}

void rhodium_radio_control_impl::_set_tx_fe_connection(const std::string& conn)
{
    RFNOC_LOG_TRACE("set_tx_fe_connection(conn=" << conn << ")");
    if (conn != _tx_fe_connection) {
        _tx_fe_core->set_mux(conn);
        _tx_fe_connection = conn;
    }
}

void rhodium_radio_control_impl::_set_rx_fe_connection(const std::string& conn)
{
    RFNOC_LOG_TRACE("set_rx_fe_connection(conn=" << conn << ")");
    if (conn != _rx_fe_connection) {
        _rx_fe_core->set_fe_connection(conn);
        _rx_fe_connection = conn;
    }
}

std::string rhodium_radio_control_impl::_get_tx_fe_connection() const
{
    return _tx_fe_connection;
}

std::string rhodium_radio_control_impl::_get_rx_fe_connection() const
{
    return _rx_fe_connection;
}

double rhodium_radio_control_impl::set_tx_frequency(const double freq, const size_t chan)
{
    RFNOC_LOG_TRACE("set_tx_frequency(f=" << freq << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    const auto old_freq        = get_tx_frequency(0);
    double coerced_target_freq = uhd::clip(freq, RHODIUM_MIN_FREQ, RHODIUM_MAX_FREQ);

    if (freq != coerced_target_freq) {
        RFNOC_LOG_DEBUG("Requested frequency is outside supported range. Coercing to "
                        << coerced_target_freq);
    }

    const bool is_highband = !_is_tx_lowband(coerced_target_freq);

    const double target_lo_freq =
        is_highband ? coerced_target_freq : _get_lowband_lo_freq() - coerced_target_freq;
    const double actual_lo_freq = set_tx_lo_freq(target_lo_freq, RHODIUM_LO1, chan);
    const double coerced_freq   = is_highband ? actual_lo_freq
                                            : _get_lowband_lo_freq() - actual_lo_freq;
    const auto conn = is_highband ? TX_FE_CONNECTION_HIGHBAND : TX_FE_CONNECTION_LOWBAND;

    // update the cached frequency value now so calls to set gain and update
    // switches will read the new frequency
    radio_control_impl::set_tx_frequency(coerced_freq, chan);

    _set_tx_fe_connection(conn);
    set_tx_gain(radio_control_impl::get_tx_gain(chan), 0);

    if (_get_highband_spur_reduction_enabled(TX_DIRECTION)) {
        if (_get_timed_command_enabled()
            and _is_tx_lowband(old_freq) != not is_highband) {
            RFNOC_LOG_WARNING(
                "Timed tuning commands that transition between lowband and highband, 450 "
                "MHz, do not function correctly when highband_spur_reduction is enabled! "
                "Disable highband_spur_reduction or avoid using timed tuning commands.");
        }
        RFNOC_LOG_TRACE("TX Lowband LO is " << (is_highband ? "disabled" : "enabled"));
        _rpcc->notify_with_token(_rpc_prefix + "enable_tx_lowband_lo", (!is_highband));
    }
    _update_tx_freq_switches(coerced_freq);
    const bool enable_corrections = is_highband
                                    and (get_tx_lo_source(RHODIUM_LO1, 0) == "internal");
    _update_corrections(actual_lo_freq, TX_DIRECTION, enable_corrections);
    // if TX lowband/highband changed and antenna is TX/RX,
    // the ATR and SW1 need to be updated
    _update_tx_output_switches(get_tx_antenna(0));
    _update_atr(get_tx_antenna(0), TX_DIRECTION);

    return coerced_freq;
}

double rhodium_radio_control_impl::set_rx_frequency(const double freq, const size_t chan)
{
    RFNOC_LOG_TRACE("set_rx_frequency(f=" << freq << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    const auto old_freq        = get_rx_frequency(0);
    double coerced_target_freq = uhd::clip(freq, RHODIUM_MIN_FREQ, RHODIUM_MAX_FREQ);

    if (freq != coerced_target_freq) {
        RFNOC_LOG_DEBUG("Requested frequency is outside supported range. Coercing to "
                        << coerced_target_freq);
    }

    const bool is_highband = !_is_rx_lowband(coerced_target_freq);

    const double target_lo_freq =
        is_highband ? coerced_target_freq : _get_lowband_lo_freq() - coerced_target_freq;
    const double actual_lo_freq = set_rx_lo_freq(target_lo_freq, RHODIUM_LO1, chan);
    const double coerced_freq   = is_highband ? actual_lo_freq
                                            : _get_lowband_lo_freq() - actual_lo_freq;
    const auto conn = is_highband ? RX_FE_CONNECTION_HIGHBAND : RX_FE_CONNECTION_LOWBAND;

    // update the cached frequency value now so calls to set gain and update
    // switches will read the new frequency
    radio_control_impl::set_rx_frequency(coerced_freq, chan);

    _set_rx_fe_connection(conn);
    set_rx_gain(radio_control_impl::get_rx_gain(chan), 0);

    if (_get_highband_spur_reduction_enabled(RX_DIRECTION)) {
        if (_get_timed_command_enabled()
            and _is_rx_lowband(old_freq) != not is_highband) {
            RFNOC_LOG_WARNING(
                "Timed tuning commands that transition between lowband and highband, 450 "
                "MHz, do not function correctly when highband_spur_reduction is enabled! "
                "Disable highband_spur_reduction or avoid using timed tuning commands.");
        }
        RFNOC_LOG_TRACE("RX Lowband LO is " << (is_highband ? "disabled" : "enabled"));
        _rpcc->notify_with_token(_rpc_prefix + "enable_rx_lowband_lo", (!is_highband));
    }
    _update_rx_freq_switches(coerced_freq);
    const bool enable_corrections = is_highband
                                    and (get_rx_lo_source(RHODIUM_LO1, 0) == "internal");
    _update_corrections(actual_lo_freq, RX_DIRECTION, enable_corrections);

    return coerced_freq;
}

void rhodium_radio_control_impl::set_tx_tune_args(
    const uhd::device_addr_t& args, const size_t chan)
{
    UHD_ASSERT_THROW(chan == 0);
    _tune_args[uhd::TX_DIRECTION] = args;
}

void rhodium_radio_control_impl::set_rx_tune_args(
    const uhd::device_addr_t& args, const size_t chan)
{
    UHD_ASSERT_THROW(chan == 0);
    _tune_args[uhd::RX_DIRECTION] = args;
}

double rhodium_radio_control_impl::set_tx_gain(const double gain, const size_t chan)
{
    RFNOC_LOG_TRACE("set_tx_gain(gain=" << gain << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    auto freq  = this->get_tx_frequency(chan);
    auto index = get_tx_gain_range(chan).clip(gain);

    auto old_band = _is_tx_lowband(_tx_frequency_at_last_gain_write)
                        ? rhodium_cpld_ctrl::gain_band_t::LOW
                        : rhodium_cpld_ctrl::gain_band_t::HIGH;
    auto new_band = _is_tx_lowband(freq) ? rhodium_cpld_ctrl::gain_band_t::LOW
                                         : rhodium_cpld_ctrl::gain_band_t::HIGH;

    // The CPLD requires a rewrite of the gain control command on a change of lowband or
    // highband
    if (radio_control_impl::get_tx_gain(chan) != index or old_band != new_band) {
        RFNOC_LOG_TRACE("Writing new TX gain index: " << index);
        _cpld->set_gain_index(index, new_band, TX_DIRECTION);
        _tx_frequency_at_last_gain_write = freq;
        radio_control_impl::set_tx_gain(index, chan);
    } else {
        RFNOC_LOG_TRACE(
            "No change in index or band, skipped writing TX gain index: " << index);
    }

    return index;
}

double rhodium_radio_control_impl::set_rx_gain(const double gain, const size_t chan)
{
    RFNOC_LOG_TRACE("set_rx_gain(gain=" << gain << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);

    auto freq  = this->get_rx_frequency(chan);
    auto index = get_rx_gain_range(chan).clip(gain);

    auto old_band = _is_rx_lowband(_rx_frequency_at_last_gain_write)
                        ? rhodium_cpld_ctrl::gain_band_t::LOW
                        : rhodium_cpld_ctrl::gain_band_t::HIGH;
    auto new_band = _is_rx_lowband(freq) ? rhodium_cpld_ctrl::gain_band_t::LOW
                                         : rhodium_cpld_ctrl::gain_band_t::HIGH;

    // The CPLD requires a rewrite of the gain control command on a change of lowband or
    // highband
    if (radio_control_impl::get_rx_gain(chan) != index or old_band != new_band) {
        RFNOC_LOG_TRACE("Writing new RX gain index: " << index);
        _cpld->set_gain_index(index, new_band, RX_DIRECTION);
        _rx_frequency_at_last_gain_write = freq;
        radio_control_impl::set_rx_gain(index, chan);
    } else {
        RFNOC_LOG_TRACE(
            "No change in index or band, skipped writing RX gain index: " << index);
    }

    return index;
}

void rhodium_radio_control_impl::_identify_with_leds(double identify_duration)
{
    auto duration_ms = static_cast<uint64_t>(identify_duration * 1000);
    auto end_time =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);
    bool led_state = true;
    {
        std::lock_guard<std::mutex> lock(_ant_mutex);
        while (std::chrono::steady_clock::now() < end_time) {
            auto atr = led_state ? (LED_RX | LED_RX2 | LED_TX) : 0;
            _gpio->set_atr_reg(gpio_atr::ATR_REG_IDLE, atr, RHODIUM_GPIO_MASK);
            led_state = !led_state;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }
    _update_atr(get_tx_antenna(0), TX_DIRECTION);
    _update_atr(get_rx_antenna(0), RX_DIRECTION);
}

void rhodium_radio_control_impl::_update_atr(
    const std::string& ant, const direction_t dir)
{
    // This function updates sw10 based on the value of both antennas, so we
    // use a mutex to prevent other calls in this class instance from running
    // at the same time.
    std::lock_guard<std::mutex> lock(_ant_mutex);

    RFNOC_LOG_TRACE(
        "Updating ATRs for " << ((dir == RX_DIRECTION) ? "RX" : "TX") << " to " << ant);

    const auto rx_ant  = (dir == RX_DIRECTION) ? ant : get_rx_antenna(0);
    const auto tx_ant  = (dir == TX_DIRECTION) ? ant : get_tx_antenna(0);
    const auto sw10_tx = _is_tx_lowband(get_tx_frequency(0)) ? SW10_FROMTXLOWBAND
                                                             : SW10_FROMTXHIGHBAND;


    const uint32_t atr_idle = SW10_ISOLATION;

    const uint32_t atr_rx = [rx_ant] {
        if (rx_ant == "TX/RX") {
            return SW10_TORX | LED_RX;
        } else if (rx_ant == "RX2") {
            return SW10_ISOLATION | LED_RX2;
        } else {
            return SW10_ISOLATION;
        }
    }();

    const uint32_t atr_tx = (tx_ant == "TX/RX") ? (sw10_tx | LED_TX) : SW10_ISOLATION;

    const uint32_t atr_dx = [tx_ant, rx_ant, sw10_tx] {
        uint32_t sw10_return;
        if (tx_ant == "TX/RX") {
            // if both channels are set to TX/RX, TX will override
            sw10_return = sw10_tx | LED_TX;
        } else if (rx_ant == "TX/RX") {
            sw10_return = SW10_TORX | LED_RX;
        } else {
            sw10_return = SW10_ISOLATION;
        }
        sw10_return |= (rx_ant == "RX2") ? LED_RX2 : 0;
        return sw10_return;
    }();

    _gpio->set_atr_reg(gpio_atr::ATR_REG_IDLE, atr_idle, RHODIUM_GPIO_MASK);
    _gpio->set_atr_reg(gpio_atr::ATR_REG_RX_ONLY, atr_rx, RHODIUM_GPIO_MASK);
    _gpio->set_atr_reg(gpio_atr::ATR_REG_TX_ONLY, atr_tx, RHODIUM_GPIO_MASK);
    _gpio->set_atr_reg(gpio_atr::ATR_REG_FULL_DUPLEX, atr_dx, RHODIUM_GPIO_MASK);

    RFNOC_LOG_TRACE(
        str(boost::format("Wrote ATR registers i:0x%02X, r:0x%02X, t:0x%02X, d:0x%02X")
            % atr_idle % atr_rx % atr_tx % atr_dx));

    if (dir == RX_DIRECTION) {
        radio_control_impl::set_rx_antenna(ant, 0);
    } else {
        radio_control_impl::set_tx_antenna(ant, 0);
    }
}

void rhodium_radio_control_impl::_update_corrections(
    const double freq, const direction_t dir, const bool enable)
{
    const std::string fe_path_part = dir == RX_DIRECTION ? "rx_fe_corrections"
                                                         : "tx_fe_corrections";
    const fs_path fe_corr_path = FE_PATH / fe_path_part / 0;

    if (enable) {
        const std::vector<uint8_t> db_serial_u8 = get_db_eeprom().count("serial")
                                                      ? get_db_eeprom().at("serial")
                                                      : std::vector<uint8_t>();
        const std::string db_serial =
            db_serial_u8.empty() ? "unknown"
                                 : std::string(db_serial_u8.begin(), db_serial_u8.end());
        RFNOC_LOG_DEBUG("Loading any available frontend corrections for "
                        << ((dir == RX_DIRECTION) ? "RX" : "TX") << " at " << freq);
        if (dir == RX_DIRECTION) {
            apply_rx_fe_corrections(get_tree(), db_serial, fe_corr_path, freq);
        } else {
            apply_tx_fe_corrections(get_tree(), db_serial, fe_corr_path, freq);
        }
    } else {
        RFNOC_LOG_DEBUG("Disabling frontend corrections for "
                        << ((dir == RX_DIRECTION) ? "RX" : "TX"));
        if (dir == RX_DIRECTION) {
            _rx_fe_core->set_iq_balance(rx_frontend_core_3000::DEFAULT_IQ_BALANCE_VALUE);
        } else {
            _tx_fe_core->set_dc_offset(tx_frontend_core_200::DEFAULT_DC_OFFSET_VALUE);
            _tx_fe_core->set_iq_balance(tx_frontend_core_200::DEFAULT_IQ_BALANCE_VALUE);
        }
    }
}

bool rhodium_radio_control_impl::_get_spur_dodging_enabled(uhd::direction_t dir) const
{
    // get the current tune_arg for spur_dodging
    // if the tune_arg doesn't exist, use the radio block argument instead
    const std::string spur_dodging_arg = _tune_args.at(dir).cast<std::string>(
        SPUR_DODGING_PROP_NAME, _spur_dodging_mode.get());

    RFNOC_LOG_TRACE("_get_spur_dodging_enabled returning " << spur_dodging_arg);
    if (spur_dodging_arg == "enabled") {
        return true;
    } else if (spur_dodging_arg == "disabled") {
        return false;
    } else {
        const std::string err_msg = str(
            boost::format(
                "Invalid spur_dodging argument: %s Valid options are [enabled, disabled]")
            % spur_dodging_arg);
        RFNOC_LOG_ERROR(err_msg);
        throw uhd::value_error(err_msg);
    }
}

double rhodium_radio_control_impl::_get_spur_dodging_threshold(uhd::direction_t dir) const
{
    // get the current tune_arg for spur_dodging_threshold
    // if the tune_arg doesn't exist, use the radio block argument instead
    const double threshold = _tune_args.at(dir).cast<double>(
        SPUR_DODGING_THRESHOLD_PROP_NAME, _spur_dodging_threshold.get());
    RFNOC_LOG_TRACE("_get_spur_dodging_threshold returning " << threshold);
    return threshold;
}

bool rhodium_radio_control_impl::_get_highband_spur_reduction_enabled(
    uhd::direction_t dir) const
{
    const std::string highband_spur_reduction_arg = _tune_args.at(dir).cast<std::string>(
        HIGHBAND_SPUR_REDUCTION_PROP_NAME, _highband_spur_reduction_mode.get());

    RFNOC_LOG_TRACE(__func__ << " returning " << highband_spur_reduction_arg);
    if (highband_spur_reduction_arg == "enabled") {
        return true;
    } else if (highband_spur_reduction_arg == "disabled") {
        return false;
    } else {
        throw uhd::value_error(
            str(boost::format("Invalid highband_spur_reduction argument: %s Valid "
                              "options are [enabled, disabled]")
                % highband_spur_reduction_arg));
    }
}

bool rhodium_radio_control_impl::_get_timed_command_enabled() const
{
    return get_command_time(0) != time_spec_t::ASAP;
}

std::vector<std::string> rhodium_radio_control_impl::get_tx_antennas(const size_t) const
{
    return RHODIUM_TX_ANTENNAS;
}

std::vector<std::string> rhodium_radio_control_impl::get_rx_antennas(const size_t) const
{
    return RHODIUM_RX_ANTENNAS;
}

uhd::freq_range_t rhodium_radio_control_impl::get_tx_frequency_range(const size_t) const
{
    return meta_range_t(RHODIUM_MIN_FREQ, RHODIUM_MAX_FREQ, 1.0);
}

uhd::freq_range_t rhodium_radio_control_impl::get_rx_frequency_range(const size_t) const
{
    return meta_range_t(RHODIUM_MIN_FREQ, RHODIUM_MAX_FREQ, 1.0);
}

uhd::gain_range_t rhodium_radio_control_impl::get_tx_gain_range(const size_t) const
{
    return gain_range_t(TX_MIN_GAIN, TX_MAX_GAIN, TX_GAIN_STEP);
}

uhd::gain_range_t rhodium_radio_control_impl::get_rx_gain_range(const size_t) const
{
    return gain_range_t(RX_MIN_GAIN, RX_MAX_GAIN, RX_GAIN_STEP);
}

uhd::meta_range_t rhodium_radio_control_impl::get_tx_bandwidth_range(size_t) const
{
    return meta_range_t(RHODIUM_DEFAULT_BANDWIDTH, RHODIUM_DEFAULT_BANDWIDTH);
}

uhd::meta_range_t rhodium_radio_control_impl::get_rx_bandwidth_range(size_t) const
{
    return meta_range_t(RHODIUM_DEFAULT_BANDWIDTH, RHODIUM_DEFAULT_BANDWIDTH);
}


/**************************************************************************
 * Radio Identification API Calls
 *************************************************************************/
size_t rhodium_radio_control_impl::get_chan_from_dboard_fe(
    const std::string& fe, const direction_t /* dir */
    ) const
{
    UHD_ASSERT_THROW(boost::lexical_cast<size_t>(fe) == 0);
    return 0;
}

std::string rhodium_radio_control_impl::get_dboard_fe_from_chan(
    const size_t chan, const direction_t /* dir */
    ) const
{
    UHD_ASSERT_THROW(chan == 0);
    return "0";
}

std::string rhodium_radio_control_impl::get_fe_name(
    const size_t, const uhd::direction_t) const
{
    return RHODIUM_FE_NAME;
}

/******************************************************************************
 * Calibration Identification API Calls
 *****************************************************************************/
void rhodium_radio_control_impl::set_tx_dc_offset(
    const std::complex<double>& offset, size_t)
{
    _tx_fe_core->set_dc_offset(offset);
}

meta_range_t rhodium_radio_control_impl::get_tx_dc_offset_range(size_t) const
{
    return get_tree()
        ->access<meta_range_t>(FE_PATH / "tx_fe_corrections" / 0 / "dc_offset/range")
        .get();
}

void rhodium_radio_control_impl::set_tx_iq_balance(
    const std::complex<double>& correction, size_t)
{
    _tx_fe_core->set_iq_balance(correction);
}

void rhodium_radio_control_impl::set_rx_dc_offset(const bool enb, size_t)
{
    _rx_fe_core->set_dc_offset_auto(enb);
}

void rhodium_radio_control_impl::set_rx_dc_offset(
    const std::complex<double>& offset, size_t)
{
    _rx_fe_core->set_dc_offset(offset);
}

meta_range_t rhodium_radio_control_impl::get_rx_dc_offset_range(size_t) const
{
    return get_tree()
        ->access<meta_range_t>(FE_PATH / "rx_fe_corrections" / 0 / "dc_offset/range")
        .get();
}

void rhodium_radio_control_impl::set_rx_iq_balance(
    const std::complex<double>& correction, size_t)
{
    _rx_fe_core->set_iq_balance(correction);
}

/**************************************************************************
 * GPIO Controls
 *************************************************************************/
std::vector<std::string> rhodium_radio_control_impl::get_gpio_banks() const
{
    return {RHODIUM_FPGPIO_BANK};
}

void rhodium_radio_control_impl::set_gpio_attr(
    const std::string& bank, const std::string& attr, const uint32_t value)
{
    if (bank != RHODIUM_FPGPIO_BANK) {
        RFNOC_LOG_ERROR("Invalid GPIO bank: " << bank);
        throw uhd::key_error("Invalid GPIO bank!");
    }
    if (!gpio_atr::gpio_attr_rev_map.count(attr)) {
        RFNOC_LOG_ERROR("Invalid GPIO attr: " << attr);
        throw uhd::key_error("Invalid GPIO attr!");
    }

    const gpio_atr::gpio_attr_t gpio_attr = gpio_atr::gpio_attr_rev_map.at(attr);

    if (gpio_attr == gpio_atr::GPIO_READBACK) {
        RFNOC_LOG_WARNING("Cannot set READBACK attr.");
        return;
    }

    _fp_gpio->set_gpio_attr(gpio_attr, value);
}

uint32_t rhodium_radio_control_impl::get_gpio_attr(
    const std::string& bank, const std::string& attr)
{
    if (bank != RHODIUM_FPGPIO_BANK) {
        RFNOC_LOG_ERROR("Invalid GPIO bank: " << bank);
        throw uhd::key_error("Invalid GPIO bank!");
    }

    return _fp_gpio->get_attr_reg(usrp::gpio_atr::gpio_attr_rev_map.at(attr));
}

/******************************************************************************
 * EEPROM API
 *****************************************************************************/
void rhodium_radio_control_impl::set_db_eeprom(const eeprom_map_t& db_eeprom)
{
    const size_t db_idx = get_block_id().get_block_count();
    _rpcc->notify_with_token("set_db_eeprom", db_idx, db_eeprom);
    _db_eeprom = this->_rpcc->request_with_token<eeprom_map_t>("get_db_eeprom", db_idx);
}

eeprom_map_t rhodium_radio_control_impl::get_db_eeprom()
{
    return _db_eeprom;
}

/**************************************************************************
 * Sensor API
 *************************************************************************/
std::vector<std::string> rhodium_radio_control_impl::get_rx_sensor_names(size_t) const
{
    return _rx_sensor_names;
}

sensor_value_t rhodium_radio_control_impl::get_rx_sensor(
    const std::string& name, size_t chan)
{
    if (!uhd::has(_rx_sensor_names, name)) {
        RFNOC_LOG_ERROR("Invalid RX sensor name: " << name);
        throw uhd::key_error("Invalid RX sensor name!");
    }
    if (name == "lo_locked") {
        return sensor_value_t(
            "all_los", this->get_lo_lock_status(RX_DIRECTION), "locked", "unlocked");
    }
    return sensor_value_t(_rpcc->request_with_token<sensor_value_t::sensor_map_t>(
        _rpc_prefix + "get_sensor", "RX", name, chan));
}

std::vector<std::string> rhodium_radio_control_impl::get_tx_sensor_names(size_t) const
{
    return _tx_sensor_names;
}

sensor_value_t rhodium_radio_control_impl::get_tx_sensor(
    const std::string& name, size_t chan)
{
    if (!uhd::has(_tx_sensor_names, name)) {
        RFNOC_LOG_ERROR("Invalid TX sensor name: " << name);
        throw uhd::key_error("Invalid TX sensor name!");
    }
    if (name == "lo_locked") {
        return sensor_value_t(
            "all_los", this->get_lo_lock_status(TX_DIRECTION), "locked", "unlocked");
    }
    return sensor_value_t(_rpcc->request_with_token<sensor_value_t::sensor_map_t>(
        _rpc_prefix + "get_sensor", "TX", name, chan));
}

bool rhodium_radio_control_impl::get_lo_lock_status(const direction_t dir) const
{
    return (dir == RX_DIRECTION) ? _rx_lo->get_lock_status() : _tx_lo->get_lock_status();
}

// Register the block
UHD_RFNOC_BLOCK_REGISTER_FOR_DEVICE_DIRECT(
    rhodium_radio_control, RADIO_BLOCK, N320, "Radio", true, "radio_clk", "bus_clk");

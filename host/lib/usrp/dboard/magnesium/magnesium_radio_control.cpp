//
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "magnesium_radio_control.hpp"
#include "magnesium_constants.hpp"
#include "magnesium_gain_table.hpp"
#include <uhd/exception.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
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

/**************************************************************************
 * ADF4351 Controls
 *************************************************************************/
/*!
 * \param lo_iface Reference to the LO object
 * \param freq Frequency (in Hz) of the tone to be generated from the LO
 * \param ref_clock_freq Frequency (in Hz) of the reference clock at the
 *                       PLL input of the LO
 * \param int_n_mode Integer-N mode on or off
 */
double _lo_set_frequency(adf435x_iface::sptr lo_iface,
    const double freq,
    const double ref_clock_freq,
    const bool int_n_mode)
{
    UHD_LOG_TRACE("MG/ADF4351",
        "Attempting to tune low band LO to " << freq << " Hz with ref clock freq "
                                             << ref_clock_freq);
    lo_iface->set_feedback_select(adf435x_iface::FB_SEL_DIVIDED);
    lo_iface->set_reference_freq(ref_clock_freq);
    lo_iface->set_prescaler(adf435x_iface::PRESCALER_4_5);
    const double actual_freq = lo_iface->set_frequency(freq, int_n_mode);
    lo_iface->set_output_power(
        adf435x_iface::RF_OUTPUT_A, adf435x_iface::OUTPUT_POWER_2DBM);
    lo_iface->set_output_power(
        adf435x_iface::RF_OUTPUT_B, adf435x_iface::OUTPUT_POWER_2DBM);
    lo_iface->set_charge_pump_current(adf435x_iface::CHARGE_PUMP_CURRENT_0_31MA);
    return actual_freq;
}

/*! Configure and enable LO
 *
 * Will tune it to requested frequency and enable outputs.
 *
 * \param lo_iface Reference to the LO object
 * \param lo_freq Frequency (in Hz) of the tone to be generated from the LO
 * \param ref_clock_freq Frequency (in Hz) of the reference clock at the
 *                       PLL input of the LO
 * \param int_n_mode Integer-N mode on or off
 * \returns the actual frequency the LO is running at
 */
double _lo_enable(adf435x_iface::sptr lo_iface,
    const double lo_freq,
    const double ref_clock_freq,
    const bool int_n_mode)
{
    const double actual_lo_freq =
        _lo_set_frequency(lo_iface, lo_freq, ref_clock_freq, int_n_mode);
    lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_A, true);
    lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_B, true);
    lo_iface->commit();
    return actual_lo_freq;
}

/*! Disable LO
 */
void _lo_disable(adf435x_iface::sptr lo_iface)
{
    lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_A, false);
    lo_iface->set_output_enable(adf435x_iface::RF_OUTPUT_B, false);
    lo_iface->commit();
}
} // namespace


/******************************************************************************
 * Structors
 *****************************************************************************/
magnesium_radio_control_impl::magnesium_radio_control_impl(make_args_ptr make_args)
    : radio_control_impl(std::move(make_args))
{
    RFNOC_LOG_TRACE("Entering magnesium_radio_control_impl ctor...");
    UHD_ASSERT_THROW(get_block_id().get_block_count() < 2);

    _tx_gain_profile_api = std::make_shared<rf_control::enumerated_gain_profile>(
        MAGNESIUM_GP_OPTIONS, "default", MAGNESIUM_NUM_CHANS);
    _rx_gain_profile_api = std::make_shared<rf_control::enumerated_gain_profile>(
        MAGNESIUM_GP_OPTIONS, "default", MAGNESIUM_NUM_CHANS);

    const char radio_slot_name[2] = {'A', 'B'};
    _radio_slot                   = radio_slot_name[get_block_id().get_block_count()];
    RFNOC_LOG_TRACE("Radio slot: " << _radio_slot);
    _rpc_prefix = (_radio_slot == "A") ? "db_0_" : "db_1_";
    UHD_ASSERT_THROW(get_num_input_ports() == MAGNESIUM_NUM_CHANS);
    UHD_ASSERT_THROW(get_num_output_ports() == MAGNESIUM_NUM_CHANS);
    UHD_ASSERT_THROW(get_mb_controller());
    _n310_mb_control = std::dynamic_pointer_cast<mpmd_mb_controller>(get_mb_controller());
    UHD_ASSERT_THROW(_n310_mb_control);
    _n3xx_timekeeper = std::dynamic_pointer_cast<mpmd_mb_controller::mpmd_timekeeper>(
        _n310_mb_control->get_timekeeper(0));
    UHD_ASSERT_THROW(_n3xx_timekeeper);
    _rpcc = _n310_mb_control->get_rpc_client();
    UHD_ASSERT_THROW(_rpcc);
    _mpm_compat_num = _rpcc->request<std::vector<size_t>>("get_mpm_compat_num");
    UHD_ASSERT_THROW(_mpm_compat_num.size() == 2);

    _init_defaults();
    _init_mpm();
    _init_peripherals();
    _init_prop_tree();
}

void magnesium_radio_control_impl::deinit()
{
    RFNOC_LOG_TRACE("magnesium_radio_control_impl::deinit()");
    _reset_tx_frontend(magnesium_cpld_ctrl::BOTH);
}

magnesium_radio_control_impl::~magnesium_radio_control_impl()
{
    RFNOC_LOG_TRACE("magnesium_radio_control_impl::dtor() ");
}


/******************************************************************************
 * API Calls
 *****************************************************************************/
double magnesium_radio_control_impl::set_rate(double requested_rate)
{
    meta_range_t rates;
    for (const double rate : MAGNESIUM_RADIO_RATES) {
        rates.push_back(range_t(rate));
    }

    const double rate = rates.clip(requested_rate);
    if (!math::frequencies_are_equal(requested_rate, rate)) {
        RFNOC_LOG_WARNING("Coercing requested sample rate from "
                          << (requested_rate / 1e6) << " to " << (rate / 1e6));
    }

    const double current_rate = get_tick_rate();
    if (math::frequencies_are_equal(current_rate, rate)) {
        RFNOC_LOG_DEBUG("Rate is already at " << rate << " MHz. Skipping set_rate()");
        return current_rate;
    }

    std::lock_guard<std::recursive_mutex> l(_set_lock);
    // Now commit to device. First, disable LOs.
    _lo_disable(_tx_lo);
    _lo_disable(_rx_lo);
    // DANGER ZONE! The only way we can change the master clock rate on N310 is
    // if we first change the master clock rate on side A, then side B. We can't
    // do it in the other order.
    // When we change the other radio's clock rate, however, the other radio
    // block controller doesn't know about that. So, we need to call set_rate()
    // on both radios every time we call it on any radio, unless the other radio
    // block is not participating in the graph.
    // Note: Updating the master clock rate is a no-op on the second call.
    const size_t num_dboards =
        _rpcc->request<std::vector<std::map<std::string, std::string>>>("get_dboard_info")
            .size();
    // Explicitly go and update rate on radio 0
    RFNOC_LOG_DEBUG("Setting master clock rate on DB0 to " << (rate / 1e6) << " MHz...");
    _master_clock_rate = _rpcc->request_with_token<double>(
        MAGNESIUM_TUNE_TIMEOUT, "db_0_set_master_clock_rate", rate);
    // Now go to the other side
    if (num_dboards == 2) {
        RFNOC_LOG_DEBUG(
            "Setting master clock rate on DB1 to " << (rate / 1e6) << " MHz...");
        const double sideB_rate = _rpcc->request_with_token<double>(
            MAGNESIUM_TUNE_TIMEOUT, "db_1_set_master_clock_rate", rate);
        if (!math::frequencies_are_equal(sideB_rate, _master_clock_rate)) {
            RFNOC_LOG_ERROR("set_rate(): Error updating rates. Slot A now has rate "
                            << (_master_clock_rate / 1e6) << " MHz, but slot B has "
                            << (sideB_rate / 1e6)
                            << " MHz. They should always be the same.");
            throw uhd::runtime_error("Different rates on radios 0 and 1!");
        }
    }
    RFNOC_LOG_DEBUG("Set MCR on both radios.");
    // Now, both radios are running at the new rate. Update all dependent
    // settings for this radio block. The other radio block needs to call
    // set_rate() too before it is in a valid state.
    _n3xx_timekeeper->update_tick_rate(_master_clock_rate);
    radio_control_impl::set_rate(_master_clock_rate);
    // Frequency settings apply to both channels, no loop needed. Will also
    // re-enable the lowband LOs if they were used.
    set_rx_frequency(get_rx_frequency(0), 0);
    set_tx_frequency(get_tx_frequency(0), 0);
    // Gain and bandwidth need to be looped:
    for (size_t radio_idx = 0; radio_idx < MAGNESIUM_NUM_CHANS; radio_idx++) {
        set_rx_gain(radio_control_impl::get_rx_gain(radio_idx), radio_idx);
        set_tx_gain(radio_control_impl::get_rx_gain(radio_idx), radio_idx);
        set_rx_bandwidth(get_rx_bandwidth(radio_idx), radio_idx);
        set_tx_bandwidth(get_tx_bandwidth(radio_idx), radio_idx);
    }
    set_tick_rate(_master_clock_rate);
    return _master_clock_rate;
}

void magnesium_radio_control_impl::set_tx_antenna(
    const std::string& ant, const size_t chan)
{
    if (ant != get_tx_antenna(chan)) {
        throw uhd::value_error(
            str(boost::format("[%s] Requesting invalid TX antenna value: %s")
                % get_unique_id() % ant));
    }
    // We can't actually set the TX antenna, so let's stop here.
}

void magnesium_radio_control_impl::set_rx_antenna(
    const std::string& ant, const size_t chan)
{
    UHD_ASSERT_THROW(chan <= MAGNESIUM_NUM_CHANS);
    if (std::find(MAGNESIUM_RX_ANTENNAS.begin(), MAGNESIUM_RX_ANTENNAS.end(), ant)
        == MAGNESIUM_RX_ANTENNAS.end()) {
        throw uhd::value_error(
            str(boost::format("[%s] Requesting invalid RX antenna value: %s")
                % get_unique_id() % ant));
    }
    RFNOC_LOG_TRACE("Setting RX antenna to " << ant << " for chan " << chan);
    magnesium_cpld_ctrl::chan_sel_t chan_sel = chan == 0 ? magnesium_cpld_ctrl::CHAN1
                                                         : magnesium_cpld_ctrl::CHAN2;
    _update_atr_switches(chan_sel, RX_DIRECTION, ant);

    radio_control_impl::set_rx_antenna(ant, chan);
}

double magnesium_radio_control_impl::set_tx_frequency(
    const double req_freq, const size_t chan)
{
    const double freq = MAGNESIUM_FREQ_RANGE.clip(req_freq);
    RFNOC_LOG_TRACE("set_tx_frequency(f=" << freq << ", chan=" << chan << ")");
    _desired_rf_freq[TX_DIRECTION] = freq;
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    // We need to set the switches on both channels, because they share an LO.
    // This way, if we tune channel 0 it will not put channel 1 into a bad
    // state.
    _update_tx_freq_switches(freq, _tx_bypass_amp, magnesium_cpld_ctrl::BOTH);
    const std::string ad9371_source  = this->get_tx_lo_source(MAGNESIUM_LO1, chan);
    const std::string adf4351_source = this->get_tx_lo_source(MAGNESIUM_LO2, chan);
    UHD_ASSERT_THROW(adf4351_source == "internal");
    double coerced_if_freq = freq;

    if (_map_freq_to_tx_band(_tx_band_map, freq) == tx_band::LOWBAND) {
        _is_low_band[TX_DIRECTION]    = true;
        const double desired_low_freq = MAGNESIUM_TX_IF_FREQ - freq;
        coerced_if_freq =
            this->_set_tx_lo_freq(adf4351_source, MAGNESIUM_LO2, desired_low_freq, chan)
            + freq;
        RFNOC_LOG_TRACE("coerced_if_freq = " << coerced_if_freq);
    } else {
        _is_low_band[TX_DIRECTION] = false;
        _lo_disable(_tx_lo);
    }
    // external LO required to tune at 2xdesired_frequency.
    const double desired_if_freq = (ad9371_source == "internal") ? coerced_if_freq
                                                                 : 2 * coerced_if_freq;

    this->_set_tx_lo_freq(ad9371_source, MAGNESIUM_LO1, desired_if_freq, chan);
    this->_update_freq(chan, TX_DIRECTION);
    this->_update_gain(chan, TX_DIRECTION);
    return radio_control_impl::get_tx_frequency(chan);
}

void magnesium_radio_control_impl::_update_gain(
    const size_t chan, const uhd::direction_t dir)
{
    const std::string fe = (dir == TX_DIRECTION) ? "tx_frontends" : "rx_frontends";
    const double freq    = (dir == TX_DIRECTION) ? this->get_tx_frequency(chan)
                                              : this->get_rx_frequency(chan);
    this->_set_all_gain(this->_get_all_gain(chan, dir), freq, chan, dir);
}

void magnesium_radio_control_impl::_update_freq(
    const size_t chan, const uhd::direction_t dir)
{
    const std::string ad9371_source = dir == TX_DIRECTION
                                          ? this->get_tx_lo_source(MAGNESIUM_LO1, chan)
                                          : this->get_rx_lo_source(MAGNESIUM_LO1, chan);

    const double ad9371_freq = ad9371_source == "external" ? _ad9371_freq[dir] / 2
                                                           : _ad9371_freq[dir];
    const double rf_freq = _is_low_band[dir] ? ad9371_freq - _adf4351_freq[dir]
                                             : ad9371_freq;

    RFNOC_LOG_TRACE("RF freq = " << rf_freq);
    UHD_ASSERT_THROW(freq_compare_epsilon(rf_freq) >= 0);
    UHD_ASSERT_THROW(freq_compare_epsilon(std::abs(rf_freq - _desired_rf_freq[dir]))
                     <= _master_clock_rate / 2);
    if (dir == RX_DIRECTION) {
        radio_control_impl::set_rx_frequency(rf_freq, chan);
    } else if (dir == TX_DIRECTION) {
        radio_control_impl::set_tx_frequency(rf_freq, chan);
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}

double magnesium_radio_control_impl::set_rx_frequency(
    const double req_freq, const size_t chan)
{
    const double freq = MAGNESIUM_FREQ_RANGE.clip(req_freq);
    RFNOC_LOG_TRACE("set_rx_frequency(f=" << freq << ", chan=" << chan << ")");
    _desired_rf_freq[RX_DIRECTION] = freq;
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    // We need to set the switches on both channels, because they share an LO.
    // This way, if we tune channel 0 it will not put channel 1 into a bad
    // state.
    _update_rx_freq_switches(freq, _rx_bypass_lnas, magnesium_cpld_ctrl::BOTH);
    const std::string ad9371_source  = this->get_rx_lo_source(MAGNESIUM_LO1, chan);
    const std::string adf4351_source = this->get_rx_lo_source(MAGNESIUM_LO2, chan);
    UHD_ASSERT_THROW(adf4351_source == "internal");
    double coerced_if_freq = freq;

    if (_map_freq_to_rx_band(_rx_band_map, freq) == rx_band::LOWBAND) {
        _is_low_band[RX_DIRECTION]    = true;
        const double desired_low_freq = MAGNESIUM_RX_IF_FREQ - freq;
        coerced_if_freq =
            this->_set_rx_lo_freq(adf4351_source, MAGNESIUM_LO2, desired_low_freq, chan)
            + freq;
        RFNOC_LOG_TRACE("coerced_if_freq = " << coerced_if_freq);
    } else {
        _is_low_band[RX_DIRECTION] = false;
        _lo_disable(_rx_lo);
    }
    // external LO required to tune at 2xdesired_frequency.
    const double desired_if_freq = ad9371_source == "internal" ? coerced_if_freq
                                                               : 2 * coerced_if_freq;

    this->_set_rx_lo_freq(ad9371_source, MAGNESIUM_LO1, desired_if_freq, chan);

    this->_update_freq(chan, RX_DIRECTION);
    this->_update_gain(chan, RX_DIRECTION);

    return radio_control_impl::get_rx_frequency(chan);
}

double magnesium_radio_control_impl::set_tx_bandwidth(
    const double bandwidth, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);

    if (_mpm_compat_num[0] < 4 || (_mpm_compat_num[0] == 4 && _mpm_compat_num[1] < 1)) {
        RFNOC_LOG_WARNING("Setting tx bandwidth not supported. Please upgrade MPM to a "
                          "minimum version of 4.1.");
        radio_control_impl::set_tx_bandwidth(bandwidth, 1 - chan);
        return radio_control_impl::set_tx_bandwidth(bandwidth, chan);
    }

    const auto curr_bw = get_tx_bandwidth(chan);
    if (fp_compare_epsilon<double>(curr_bw) == bandwidth) {
        return curr_bw;
    }

    // TX frontend components should be deactivated before running init_cals
    const auto tx_atr_bits =
        _cpld->get_tx_atr_bits(magnesium_cpld_ctrl::BOTH, magnesium_cpld_ctrl::IDLE);
    _reset_tx_frontend(magnesium_cpld_ctrl::BOTH);

    RFNOC_LOG_INFO(
        "Re-initializing dboard to apply bandwidth settings. This may take some time.");
    const auto ret = _ad9371->set_bandwidth(bandwidth, chan, TX_DIRECTION);
    RFNOC_LOG_INFO("Bandwidth settings applied: " + std::to_string(ret));

    // Restore TX frontend components to previous state
    _cpld->set_tx_atr_bits(
        magnesium_cpld_ctrl::BOTH, magnesium_cpld_ctrl::IDLE, tx_atr_bits);

    // Save the updated bandwidth settings for both channels
    radio_control_impl::set_tx_bandwidth(bandwidth, 1 - chan);
    return radio_control_impl::set_tx_bandwidth(ret, chan);
}

double magnesium_radio_control_impl::set_rx_bandwidth(
    const double bandwidth, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);

    if (_mpm_compat_num[0] < 4 || (_mpm_compat_num[0] == 4 && _mpm_compat_num[1] < 1)) {
        RFNOC_LOG_WARNING("Setting rx bandwidth not supported. Please upgrade MPM to a "
                          "minimum version of 4.1.");
        radio_control_impl::set_rx_bandwidth(bandwidth, 1 - chan);
        return radio_control_impl::set_rx_bandwidth(bandwidth, chan);
    }

    const auto curr_bw = get_rx_bandwidth(chan);
    if (fp_compare_epsilon<double>(curr_bw) == bandwidth) {
        return curr_bw;
    }

    // TX frontend components should be deactivated before running init_cals
    //
    // We want to avoid any TX signals coming out of the frontend. This is
    // necessary even during RX calibration.
    const auto tx_atr_bits =
        _cpld->get_tx_atr_bits(magnesium_cpld_ctrl::BOTH, magnesium_cpld_ctrl::IDLE);
    _reset_tx_frontend(magnesium_cpld_ctrl::BOTH);

    RFNOC_LOG_INFO(
        "Re-initializing dboard to apply bandwidth settings. This may take some time.");
    const auto ret = _ad9371->set_bandwidth(bandwidth, chan, RX_DIRECTION);
    RFNOC_LOG_INFO("Bandwidth settings applied: " + std::to_string(ret));

    // Restore TX frontend components to previous state
    _cpld->set_tx_atr_bits(
        magnesium_cpld_ctrl::BOTH, magnesium_cpld_ctrl::IDLE, tx_atr_bits);

    // Save the updated bandwidth settings for both channels
    radio_control_impl::set_rx_bandwidth(bandwidth, 1 - chan);
    return radio_control_impl::set_rx_bandwidth(ret, chan);
}

double magnesium_radio_control_impl::set_tx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_TRACE("set_tx_gain(gain=" << gain << ", chan=" << chan << ")");
    // First, clip to valid range
    const double clipped_gain = get_tx_gain_range(chan).clip(gain);
    if (clipped_gain != gain) {
        RFNOC_LOG_WARNING("Channel " << chan << ": Coercing TX gain from " << gain
                                     << " dB to " << clipped_gain);
    }
    const double coerced_gain =
        _set_all_gain(clipped_gain, this->get_tx_frequency(chan), chan, TX_DIRECTION);
    radio_control_impl::set_tx_gain(coerced_gain, chan);
    return coerced_gain;
}

double magnesium_radio_control_impl::_set_tx_gain(
    const std::string& name, const double gain, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_TRACE(
        "_set_tx_gain(name=" << name << ", gain=" << gain << ", chan=" << chan << ")");
    RFNOC_LOG_TRACE(
        "_set_tx_gain(name=" << name << ", gain=" << gain << ", chan=" << chan << ")");
    double clip_gain = 0;
    if (name == MAGNESIUM_GAIN1) {
        clip_gain = uhd::clip(gain, AD9371_MIN_TX_GAIN, AD9371_MAX_TX_GAIN);
        _ad9371_att[TX_DIRECTION] = clip_gain;
    } else if (name == MAGNESIUM_GAIN2) {
        clip_gain              = uhd::clip(gain, DSA_MIN_GAIN, DSA_MAX_GAIN);
        _dsa_att[TX_DIRECTION] = clip_gain;
    } else if (name == MAGNESIUM_AMP) {
        clip_gain                 = gain > 0.0 ? AMP_MAX_GAIN : AMP_MIN_GAIN;
        _amp_bypass[TX_DIRECTION] = clip_gain == 0.0;
    } else {
        throw uhd::value_error("Could not find gain element " + name);
    }
    RFNOC_LOG_TRACE("_set_tx_gain calling update gain");
    this->_set_all_gain(this->_get_all_gain(chan, TX_DIRECTION),
        this->get_tx_frequency(chan),
        chan,
        TX_DIRECTION);
    return clip_gain;
}

double magnesium_radio_control_impl::_get_tx_gain(
    const std::string& name, const size_t /*chan*/
)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    if (name == MAGNESIUM_GAIN1) {
        return _ad9371_att[TX_DIRECTION];
    } else if (name == MAGNESIUM_GAIN2) {
        return _dsa_att[TX_DIRECTION];
    } else if (name == MAGNESIUM_AMP) {
        return _amp_bypass[TX_DIRECTION] ? AMP_MIN_GAIN : AMP_MAX_GAIN;
    } else {
        throw uhd::value_error("Could not find gain element " + name);
    }
}

double magnesium_radio_control_impl::set_rx_gain(const double gain, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_TRACE("set_rx_gain(gain=" << gain << ", chan=" << chan << ")");
    // First, clip to valid range
    const double clipped_gain = get_rx_gain_range(chan).clip(gain);
    if (clipped_gain != gain) {
        RFNOC_LOG_WARNING("Channel " << chan << ": Coercing RX gain from " << gain
                                     << " dB to " << clipped_gain);
    }
    const double coerced_gain =
        _set_all_gain(clipped_gain, this->get_rx_frequency(chan), chan, RX_DIRECTION);
    radio_control_impl::set_rx_gain(coerced_gain, chan);
    return coerced_gain;
}

double magnesium_radio_control_impl::_set_rx_gain(
    const std::string& name, const double gain, const size_t chan)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_TRACE(
        "_set_rx_gain(name=" << name << ", gain=" << gain << ", chan=" << chan << ")");
    double clip_gain = 0;
    if (name == MAGNESIUM_GAIN1) {
        clip_gain = uhd::clip(gain, AD9371_MIN_RX_GAIN, AD9371_MAX_RX_GAIN);
        _ad9371_att[RX_DIRECTION] = clip_gain;
    } else if (name == MAGNESIUM_GAIN2) {
        clip_gain              = uhd::clip(gain, DSA_MIN_GAIN, DSA_MAX_GAIN);
        _dsa_att[RX_DIRECTION] = clip_gain;
    } else if (name == MAGNESIUM_AMP) {
        clip_gain                 = gain > 0.0 ? AMP_MAX_GAIN : AMP_MIN_GAIN;
        _amp_bypass[RX_DIRECTION] = clip_gain == 0.0;
    } else {
        throw uhd::value_error("Could not find gain element " + name);
    }
    RFNOC_LOG_TRACE("_set_rx_gain calling update gain");
    this->_set_all_gain(this->_get_all_gain(chan, RX_DIRECTION),
        this->get_rx_frequency(chan),
        chan,
        RX_DIRECTION);
    return clip_gain; // not really any coerced here (only clip) for individual gain
}

double magnesium_radio_control_impl::_get_rx_gain(
    const std::string& name, const size_t /*chan*/
)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);

    if (name == MAGNESIUM_GAIN1) {
        return _ad9371_att[RX_DIRECTION];
    } else if (name == MAGNESIUM_GAIN2) {
        return _dsa_att[RX_DIRECTION];
    } else if (name == MAGNESIUM_AMP) {
        return _amp_bypass[RX_DIRECTION] ? AMP_MIN_GAIN : AMP_MAX_GAIN;
    } else {
        throw uhd::value_error("Could not find gain element " + name);
    }
}

double magnesium_radio_control_impl::set_tx_gain(
    const double gain, const std::string& name, const size_t chan)
{
    if (get_tx_gain_profile(chan) == "manual") {
        if (name == "all" || name == ALL_GAINS) {
            RFNOC_LOG_ERROR("Setting overall gain is not supported in manual gain mode!");
            throw uhd::key_error(
                "Setting overall gain is not supported in manual gain mode!");
        }
        if (name != MAGNESIUM_GAIN1 && name != MAGNESIUM_GAIN2 && name != MAGNESIUM_AMP) {
            RFNOC_LOG_ERROR("Invalid TX gain name: " << name);
            throw uhd::key_error("Invalid TX gain name!");
        }
        const double coerced_gain = get_tx_gain_range(name, chan).clip(gain, true);
        if (name == MAGNESIUM_GAIN1) {
            _ad9371_att[TX_DIRECTION] = AD9371_MAX_TX_GAIN - coerced_gain;
        } else if (name == MAGNESIUM_GAIN2) {
            _dsa_set_att(AD9371_MAX_TX_GAIN - coerced_gain, chan, TX_DIRECTION);
        } else if (name == MAGNESIUM_AMP) {
            _amp_bypass[TX_DIRECTION] = (coerced_gain == AMP_MIN_GAIN);
        } else {
            throw uhd::value_error("Could not find gain element " + name);
        }
        _set_all_gain(coerced_gain /* this value doesn't actuall matter */,
            get_tx_frequency(chan),
            chan,
            TX_DIRECTION);
        return coerced_gain;
    }

    if (name == "all" || name == ALL_GAINS) {
        return set_tx_gain(gain, chan);
    }
    RFNOC_LOG_ERROR("Setting individual TX gains is only supported in manual gain mode!");
    throw uhd::key_error(
        "Setting individual TX gains is only supported in manual gain mode!");
}

double magnesium_radio_control_impl::set_rx_gain(
    const double gain, const std::string& name, const size_t chan)
{
    if (get_rx_gain_profile(chan) == "manual") {
        if (name == "all" || name == ALL_GAINS) {
            RFNOC_LOG_ERROR("Setting overall gain is not supported in manual gain mode!");
            throw uhd::key_error(
                "Setting overall gain is not supported in manual gain mode!");
        }
        if (name != MAGNESIUM_GAIN1 && name != MAGNESIUM_GAIN2 && name != MAGNESIUM_AMP) {
            RFNOC_LOG_ERROR("Invalid RX gain name: " << name);
            throw uhd::key_error("Invalid RX gain name!");
        }
        const double coerced_gain = get_rx_gain_range(name, chan).clip(gain, true);
        if (name == MAGNESIUM_GAIN1) {
            _ad9371_att[RX_DIRECTION] = AD9371_MAX_RX_GAIN - coerced_gain;
        } else if (name == MAGNESIUM_GAIN2) {
            _dsa_set_att(AD9371_MAX_RX_GAIN - coerced_gain, chan, RX_DIRECTION);
        } else if (name == MAGNESIUM_AMP) {
            _amp_bypass[RX_DIRECTION] = (coerced_gain == AMP_MIN_GAIN);
        } else {
            throw uhd::value_error("Could not find gain element " + name);
        }
        _set_all_gain(coerced_gain /* this value doesn't actuall matter */,
            get_rx_frequency(chan),
            chan,
            RX_DIRECTION);
        return coerced_gain;
    }

    if (name == "all" || name == ALL_GAINS) {
        return set_rx_gain(gain, chan);
    }
    RFNOC_LOG_ERROR("Setting individual RX gains is only supported in manual gain mode!");
    throw uhd::key_error(
        "Setting individual RX gains is only supported in manual gain mode!");
}

std::vector<std::string> magnesium_radio_control_impl::get_tx_antennas(const size_t) const
{
    return {"TX/RX"};
}

std::vector<std::string> magnesium_radio_control_impl::get_rx_antennas(const size_t) const
{
    return MAGNESIUM_RX_ANTENNAS;
}

uhd::freq_range_t magnesium_radio_control_impl::get_tx_frequency_range(const size_t) const
{
    return meta_range_t(MAGNESIUM_MIN_FREQ, MAGNESIUM_MAX_FREQ, 1.0);
}

uhd::freq_range_t magnesium_radio_control_impl::get_rx_frequency_range(const size_t) const
{
    return meta_range_t(MAGNESIUM_MIN_FREQ, MAGNESIUM_MAX_FREQ, 1.0);
}

std::vector<std::string> magnesium_radio_control_impl::get_tx_gain_names(
    const size_t) const
{
    return {MAGNESIUM_GAIN1, MAGNESIUM_GAIN2, MAGNESIUM_AMP};
}

std::vector<std::string> magnesium_radio_control_impl::get_rx_gain_names(
    const size_t) const
{
    return {MAGNESIUM_GAIN1, MAGNESIUM_GAIN2, MAGNESIUM_AMP};
}

double magnesium_radio_control_impl::get_tx_gain(
    const std::string& name, const size_t chan)
{
    if (name == MAGNESIUM_GAIN1 || name == MAGNESIUM_GAIN2 || name == MAGNESIUM_AMP) {
        return _get_tx_gain(name, chan);
    }
    if (name == "all" || name == ALL_GAINS) {
        return radio_control_impl::get_tx_gain(chan);
    }
    RFNOC_LOG_ERROR("Invalid TX gain name: " << name);
    throw uhd::key_error("Invalid TX gain name!");
}

double magnesium_radio_control_impl::get_rx_gain(
    const std::string& name, const size_t chan)
{
    if (name == MAGNESIUM_GAIN1 || name == MAGNESIUM_GAIN2 || name == MAGNESIUM_AMP) {
        return _get_rx_gain(name, chan);
    }
    if (name == "all" || name == ALL_GAINS) {
        return radio_control_impl::get_rx_gain(chan);
    }
    RFNOC_LOG_ERROR("Invalid RX gain name: " << name);
    throw uhd::key_error("Invalid RX gain name!");
}

uhd::gain_range_t magnesium_radio_control_impl::get_tx_gain_range(const size_t chan) const
{
    if (get_tx_gain_profile(chan) == "manual") {
        return meta_range_t(0.0, 0.0, 0.0);
    }
    return meta_range_t(ALL_TX_MIN_GAIN, ALL_TX_MAX_GAIN, ALL_TX_GAIN_STEP);
}

uhd::gain_range_t magnesium_radio_control_impl::get_tx_gain_range(
    const std::string& name, const size_t chan) const
{
    if (get_tx_gain_profile(chan) == "manual") {
        if (name == "all" || name == ALL_GAINS) {
            return meta_range_t(0.0, 0.0, 0.0);
        }
        if (name == MAGNESIUM_GAIN1) {
            return meta_range_t(
                AD9371_MIN_TX_GAIN, AD9371_MAX_TX_GAIN, AD9371_TX_GAIN_STEP);
        }
        if (name == MAGNESIUM_GAIN2) {
            return meta_range_t(DSA_MIN_GAIN, DSA_MAX_GAIN, DSA_GAIN_STEP);
        }
        if (name == MAGNESIUM_AMP) {
            return meta_range_t(AMP_MIN_GAIN, AMP_MAX_GAIN, AMP_GAIN_STEP);
        }
        RFNOC_LOG_ERROR("Invalid TX gain name: " << name);
        throw uhd::key_error("Invalid TX gain name!");
    }
    if (name == "all" || name == ALL_GAINS) {
        return get_tx_gain_range(chan);
    }
    if (name == MAGNESIUM_GAIN1 || name == MAGNESIUM_GAIN2 || name == MAGNESIUM_AMP) {
        return meta_range_t(0.0, 0.0, 0.0);
    }
    RFNOC_LOG_ERROR("Invalid TX gain name: " << name);
    throw uhd::key_error("Invalid TX gain name!");
}

uhd::gain_range_t magnesium_radio_control_impl::get_rx_gain_range(const size_t chan) const
{
    if (get_rx_gain_profile(chan) == "manual") {
        return meta_range_t(0.0, 0.0, 0.0);
    }
    return meta_range_t(ALL_RX_MIN_GAIN, ALL_RX_MAX_GAIN, ALL_RX_GAIN_STEP);
}

uhd::gain_range_t magnesium_radio_control_impl::get_rx_gain_range(
    const std::string& name, const size_t chan) const
{
    if (get_rx_gain_profile(chan) == "manual") {
        if (name == "all" || name == ALL_GAINS) {
            return meta_range_t(0.0, 0.0, 0.0);
        }
        if (name == MAGNESIUM_GAIN1) {
            return meta_range_t(
                AD9371_MIN_RX_GAIN, AD9371_MAX_RX_GAIN, AD9371_RX_GAIN_STEP);
        }
        if (name == MAGNESIUM_GAIN2) {
            return meta_range_t(DSA_MIN_GAIN, DSA_MAX_GAIN, DSA_GAIN_STEP);
        }
        if (name == MAGNESIUM_AMP) {
            return meta_range_t(AMP_MIN_GAIN, AMP_MAX_GAIN, AMP_GAIN_STEP);
        }
        RFNOC_LOG_ERROR("Invalid RX gain name: " << name);
        throw uhd::key_error("Invalid RX gain name!");
    }
    if (name == "all" || name == ALL_GAINS) {
        return get_rx_gain_range(chan);
    }
    if (name == MAGNESIUM_GAIN1 || name == MAGNESIUM_GAIN2 || name == MAGNESIUM_AMP) {
        return meta_range_t(0.0, 0.0, 0.0);
    }
    RFNOC_LOG_ERROR("Invalid RX gain name: " << name);
    throw uhd::key_error("Invalid RX gain name!");
}

meta_range_t magnesium_radio_control_impl::get_tx_bandwidth_range(size_t) const
{
    return meta_range_t(AD9371_TX_MIN_BANDWIDTH, AD9371_TX_MAX_BANDWIDTH);
}

meta_range_t magnesium_radio_control_impl::get_rx_bandwidth_range(size_t) const
{
    return meta_range_t(AD9371_TX_MIN_BANDWIDTH, AD9371_TX_MAX_BANDWIDTH);
}


/******************************************************************************
 * LO Controls
 *****************************************************************************/
std::vector<std::string> magnesium_radio_control_impl::get_rx_lo_names(
    const size_t /*chan*/
) const
{
    return std::vector<std::string>{MAGNESIUM_LO1, MAGNESIUM_LO2};
}

std::vector<std::string> magnesium_radio_control_impl::get_rx_lo_sources(
    const std::string& name, const size_t /*chan*/
) const
{
    if (name == MAGNESIUM_LO2) {
        return std::vector<std::string>{"internal"};
    } else if (name == MAGNESIUM_LO1) {
        return std::vector<std::string>{"internal", "external"};
    } else {
        throw uhd::value_error("Could not find LO stage " + name);
    }
}

freq_range_t magnesium_radio_control_impl::get_rx_lo_freq_range(
    const std::string& name, const size_t /*chan*/
) const
{
    if (name == MAGNESIUM_LO1) {
        return freq_range_t{ADF4351_MIN_FREQ, ADF4351_MAX_FREQ};
    } else if (name == MAGNESIUM_LO2) {
        return freq_range_t{AD9371_MIN_FREQ, AD9371_MAX_FREQ};
    } else {
        throw uhd::value_error("Could not find LO stage " + name);
    }
}

void magnesium_radio_control_impl::set_rx_lo_source(
    const std::string& src, const std::string& name, const size_t /*chan*/
)
{
    // TODO: checking what options are there
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_TRACE("Setting RX LO " << name << " to " << src);

    if (name == MAGNESIUM_LO1) {
        _ad9371->set_lo_source(src, RX_DIRECTION);
    } else {
        RFNOC_LOG_ERROR(
            "RX LO " << name << " does not support setting source to " << src);
    }
}

const std::string magnesium_radio_control_impl::get_rx_lo_source(
    const std::string& name, const size_t /*chan*/
)
{
    if (name == MAGNESIUM_LO1) {
        // TODO: should we use this from cache?
        return _ad9371->get_lo_source(RX_DIRECTION);
    }
    return "internal";
}

double magnesium_radio_control_impl::_set_rx_lo_freq(const std::string source,
    const std::string name,
    const double freq,
    const size_t chan)
{
    double coerced_lo_freq = freq;
    if (source != "internal") {
        RFNOC_LOG_WARNING(
            "LO source is not internal. This set frequency will be ignored");
        if (name == MAGNESIUM_LO1) {
            // handle ad9371 external LO case
            coerced_lo_freq            = freq;
            _ad9371_freq[RX_DIRECTION] = coerced_lo_freq;
        }
    } else {
        if (name == MAGNESIUM_LO1) {
            coerced_lo_freq            = _ad9371->set_frequency(freq, chan, RX_DIRECTION);
            _ad9371_freq[RX_DIRECTION] = coerced_lo_freq;
        } else if (name == MAGNESIUM_LO2) {
            coerced_lo_freq = _lo_enable(_rx_lo, freq, _master_clock_rate, false);
            _adf4351_freq[RX_DIRECTION] = coerced_lo_freq;
        } else {
            RFNOC_LOG_WARNING("There's no LO with this name of "
                              << name
                              << " in the system. This set rx lo freq will be ignored");
        };
    }
    return coerced_lo_freq;
}

double magnesium_radio_control_impl::set_rx_lo_freq(
    double freq, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_rx_lo_freq(freq=" << freq << ", name=" << name << ")");
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    std::string source           = this->get_rx_lo_source(name, chan);
    const double coerced_lo_freq = this->_set_rx_lo_freq(source, name, freq, chan);
    this->_update_freq(chan, RX_DIRECTION);
    this->_update_gain(chan, RX_DIRECTION);
    return coerced_lo_freq;
}

double magnesium_radio_control_impl::get_rx_lo_freq(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_rx_lo_freq(name=" << name << ")");
    std::string source = this->get_rx_lo_source(name, chan);
    if (name == MAGNESIUM_LO1) {
        return _ad9371_freq.at(RX_DIRECTION);
    } else if (name == "adf4531") {
        return _adf4351_freq.at(RX_DIRECTION);
    } else {
        RFNOC_LOG_ERROR("get_rx_lo_freq(): No such LO: " << name);
    }
    UHD_THROW_INVALID_CODE_PATH();
}

// TX LO
std::vector<std::string> magnesium_radio_control_impl::get_tx_lo_names(
    const size_t /*chan*/
) const
{
    return std::vector<std::string>{MAGNESIUM_LO1, MAGNESIUM_LO2};
}

std::vector<std::string> magnesium_radio_control_impl::get_tx_lo_sources(
    const std::string& name, const size_t /*chan*/
) const
{
    if (name == MAGNESIUM_LO2) {
        return std::vector<std::string>{"internal"};
    } else if (name == MAGNESIUM_LO1) {
        return std::vector<std::string>{"internal", "external"};
    } else {
        throw uhd::value_error("Could not find LO stage " + name);
    }
}

freq_range_t magnesium_radio_control_impl::get_tx_lo_freq_range(
    const std::string& name, const size_t /*chan*/
)
{
    if (name == MAGNESIUM_LO2) {
        return freq_range_t{ADF4351_MIN_FREQ, ADF4351_MAX_FREQ};
    } else if (name == MAGNESIUM_LO1) {
        return freq_range_t{AD9371_MIN_FREQ, AD9371_MAX_FREQ};
    } else {
        throw uhd::value_error("Could not find LO stage " + name);
    }
}

void magnesium_radio_control_impl::set_tx_lo_source(
    const std::string& src, const std::string& name, const size_t /*chan*/
)
{
    // TODO: checking what options are there
    std::lock_guard<std::recursive_mutex> l(_set_lock);
    RFNOC_LOG_TRACE("set_tx_lo_source(name=" << name << ", src=" << src << ")");
    if (name == MAGNESIUM_LO1) {
        _ad9371->set_lo_source(src, TX_DIRECTION);
    } else {
        RFNOC_LOG_ERROR(
            "TX LO " << name << " does not support setting source to " << src);
    }
}

const std::string magnesium_radio_control_impl::get_tx_lo_source(
    const std::string& name, const size_t /*chan*/
)
{
    if (name == MAGNESIUM_LO1) {
        // TODO: should we use this from cache?
        return _ad9371->get_lo_source(TX_DIRECTION);
    }
    return "internal";
}

double magnesium_radio_control_impl::_set_tx_lo_freq(const std::string source,
    const std::string name,
    const double freq,
    const size_t chan)
{
    double coerced_lo_freq = freq;
    if (source != "internal") {
        RFNOC_LOG_WARNING(
            "LO source is not internal. This set frequency will be ignored");
        if (name == MAGNESIUM_LO1) {
            // handle ad9371 external LO case
            coerced_lo_freq            = freq;
            _ad9371_freq[TX_DIRECTION] = coerced_lo_freq;
        }
    } else {
        if (name == MAGNESIUM_LO1) {
            coerced_lo_freq            = _ad9371->set_frequency(freq, chan, TX_DIRECTION);
            _ad9371_freq[TX_DIRECTION] = coerced_lo_freq;
        } else if (name == MAGNESIUM_LO2) {
            const bool int_n_mode = false;
            coerced_lo_freq = _lo_enable(_tx_lo, freq, _master_clock_rate, int_n_mode);
            _adf4351_freq[TX_DIRECTION] = coerced_lo_freq;
        } else {
            RFNOC_LOG_WARNING("There's no LO with this name of "
                              << name
                              << " in the system. This set tx lo freq will be ignored");
        };
    }
    return coerced_lo_freq;
}

double magnesium_radio_control_impl::set_tx_lo_freq(
    double freq, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_tx_lo_freq(freq=" << freq << ", name=" << name << ")");
    std::string source       = this->get_tx_lo_source(name, chan);
    const double return_freq = this->_set_tx_lo_freq(source, name, freq, chan);
    this->_update_freq(chan, TX_DIRECTION);
    this->_update_gain(chan, TX_DIRECTION);
    return return_freq;
}

double magnesium_radio_control_impl::get_tx_lo_freq(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_tx_lo_freq(name=" << name << ")");
    std::string source = this->get_tx_lo_source(name, chan);
    if (name == MAGNESIUM_LO1) {
        return _ad9371_freq[TX_DIRECTION];
    } else if (name == MAGNESIUM_LO2) {
        return _adf4351_freq[TX_DIRECTION];
    } else {
        RFNOC_LOG_ERROR("get_tx_lo_freq(): No such LO: " << name);
    };

    UHD_THROW_INVALID_CODE_PATH();
}

void magnesium_radio_control_impl::_remap_band_limits(
    const std::string band_map, const uhd::direction_t dir)
{
    const size_t dflt_band_size = (dir == RX_DIRECTION) ? _rx_band_map.size()
                                                        : _tx_band_map.size();

    std::vector<std::string> band_map_split;
    double band_lim;

    RFNOC_LOG_DEBUG("Using user specified frequency band limits");
    boost::split(band_map_split, band_map, boost::is_any_of(";"));
    if (band_map_split.size() != dflt_band_size) {
        throw uhd::runtime_error((
            boost::format(
                "size %s of given frequency band map doesn't match the required size: %s")
            % band_map_split.size() % dflt_band_size)
                                     .str());
    }
    RFNOC_LOG_DEBUG("newly used band limits: ");
    for (size_t i = 0; i < band_map_split.size(); i++) {
        try {
            band_lim = std::stod(band_map_split.at(i));
        } catch (...) {
            throw uhd::value_error(
                (boost::format("error while converting given frequency string %s "
                               "to a double value")
                    % band_map_split.at(i))
                    .str());
        }
        RFNOC_LOG_DEBUG("band " << i << " limit: " << band_lim << "Hz");
        if (dir == RX_DIRECTION)
            _rx_band_map.at(i) = band_lim;
        else
            _tx_band_map.at(i) = band_lim;
    }
}


bool magnesium_radio_control_impl::get_lo_lock_status(const direction_t dir)
{
    if (not(bool(_rpcc))) {
        RFNOC_LOG_WARNING("Reported no LO lock due to lack of RPC connection.");
        return false;
    }

    const std::string trx = (dir == RX_DIRECTION) ? "rx" : "tx";
    const size_t chan     = 0; // They're the same after all
    const double freq     = (dir == RX_DIRECTION) ? get_rx_frequency(chan)
                                              : get_tx_frequency(chan);

    bool lo_lock =
        _rpcc->request_with_token<bool>(_rpc_prefix + "get_ad9371_lo_lock", trx);
    RFNOC_LOG_TRACE("AD9371 " << trx << " LO reports lock: " << (lo_lock ? "Yes" : "No"));
    if (lo_lock and _map_freq_to_rx_band(_rx_band_map, freq) == rx_band::LOWBAND) {
        lo_lock =
            lo_lock
            && _rpcc->request_with_token<bool>(_rpc_prefix + "get_lowband_lo_lock", trx);
        RFNOC_LOG_TRACE(
            "ADF4351 " << trx << " LO reports lock: " << (lo_lock ? "Yes" : "No"));
    }

    return lo_lock;
}

/**************************************************************************
 * GPIO Controls
 *************************************************************************/
std::vector<std::string> magnesium_radio_control_impl::get_gpio_banks() const
{
    return {MAGNESIUM_FPGPIO_BANK};
}

void magnesium_radio_control_impl::set_gpio_attr(
    const std::string& bank, const std::string& attr, const uint32_t value)
{
    if (bank != MAGNESIUM_FPGPIO_BANK) {
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

uint32_t magnesium_radio_control_impl::get_gpio_attr(
    const std::string& bank, const std::string& attr)
{
    if (bank != MAGNESIUM_FPGPIO_BANK) {
        RFNOC_LOG_ERROR("Invalid GPIO bank: " << bank);
        throw uhd::key_error("Invalid GPIO bank!");
    }

    return _fp_gpio->get_attr_reg(usrp::gpio_atr::gpio_attr_rev_map.at(attr));
}

/******************************************************************************
 * EEPROM API
 *****************************************************************************/
void magnesium_radio_control_impl::set_db_eeprom(const eeprom_map_t& db_eeprom)
{
    const size_t db_idx = get_block_id().get_block_count();
    _rpcc->notify_with_token("set_db_eeprom", db_idx, db_eeprom);
}

eeprom_map_t magnesium_radio_control_impl::get_db_eeprom()
{
    const size_t db_idx = get_block_id().get_block_count();
    return this->_rpcc->request_with_token<eeprom_map_t>("get_db_eeprom", db_idx);
}

/**************************************************************************
 * Sensor API
 *************************************************************************/
std::vector<std::string> magnesium_radio_control_impl::get_rx_sensor_names(size_t) const
{
    auto sensor_names = _rpcc->request_with_token<std::vector<std::string>>(
        this->_rpc_prefix + "get_sensors", "RX");
    sensor_names.push_back("lo_locked");
    return sensor_names;
}

sensor_value_t magnesium_radio_control_impl::get_rx_sensor(
    const std::string& name, size_t chan)
{
    if (name == "lo_locked") {
        return sensor_value_t(
            "all_los", this->get_lo_lock_status(RX_DIRECTION), "locked", "unlocked");
    }
    return sensor_value_t(_rpcc->request_with_token<sensor_value_t::sensor_map_t>(
        _rpc_prefix + "get_sensor", "RX", name, chan));
}

std::vector<std::string> magnesium_radio_control_impl::get_tx_sensor_names(size_t) const
{
    auto sensor_names = _rpcc->request_with_token<std::vector<std::string>>(
        this->_rpc_prefix + "get_sensors", "TX");
    sensor_names.push_back("lo_locked");
    return sensor_names;
}

sensor_value_t magnesium_radio_control_impl::get_tx_sensor(
    const std::string& name, size_t chan)
{
    if (name == "lo_locked") {
        return sensor_value_t(
            "all_los", this->get_lo_lock_status(TX_DIRECTION), "locked", "unlocked");
    }
    return sensor_value_t(_rpcc->request_with_token<sensor_value_t::sensor_map_t>(
        _rpc_prefix + "get_sensor", "TX", name, chan));
}

/**************************************************************************
 * Filter API
 *************************************************************************/
std::vector<std::string> magnesium_radio_control_impl::get_rx_filter_names(
    const size_t chan) const
{
    UHD_ASSERT_THROW(chan < TOTAL_RADIO_PORTS);
    if (chan % 2 == 0) {
        return {"RX1_FIR", "RX1RX2_FIR"};
    } else {
        return {"RX2_FIR", "RX1RX2_FIR"};
    }
}

uhd::filter_info_base::sptr magnesium_radio_control_impl::get_rx_filter(
    const std::string& name, const size_t)
{
    if (_mpm_compat_num[0] < 4 || (_mpm_compat_num[0] == 4 && _mpm_compat_num[1] < 2)) {
        RFNOC_LOG_WARNING("Getting rx filter not supported. Please upgrade MPM to a "
                          "minimum version of 4.2.");
        return std::make_shared<uhd::digital_filter_fir<int16_t>>(
            uhd::filter_info_base::filter_type::DIGITAL_FIR_I16,
            false,
            0,
            1.0,
            1,
            1,
            32767,
            AD9371_RX_MAX_FIR_TAPS,
            std::vector<int16_t>(AD9371_RX_MAX_FIR_TAPS, 0));
    }

    const auto rv     = _ad9371->get_fir(name);
    const auto coeffs = rv.second;
    // TODO: Put gain in the digital_filter_fir
    return std::make_shared<uhd::digital_filter_fir<int16_t>>(
        uhd::filter_info_base::filter_type::DIGITAL_FIR_I16,
        false,
        0,
        1.0,
        1,
        1,
        32767,
        AD9371_RX_MAX_FIR_TAPS,
        coeffs);
}

void magnesium_radio_control_impl::set_rx_filter(
    const std::string& name, uhd::filter_info_base::sptr filter, const size_t)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);

    if (_mpm_compat_num[0] < 4 || (_mpm_compat_num[0] == 4 && _mpm_compat_num[1] < 2)) {
        RFNOC_LOG_WARNING("Setting rx filter not supported. Please upgrade MPM to a "
                          "minimum version of 4.2.");
        return;
    }

    auto fir = std::dynamic_pointer_cast<uhd::digital_filter_fir<int16_t>>(filter);
    if (fir == nullptr) {
        throw uhd::runtime_error("Invalid Filter Type for RX Filter");
    }
    if (fir->get_taps().size() != AD9371_RX_MAX_FIR_TAPS) {
        throw uhd::runtime_error("AD937x RX Filter Taps must be "
                                 + std::to_string(AD9371_RX_MAX_FIR_TAPS)
                                 + " taps long!");
    }
    // TODO: Use gain in the digital_filter_fir
    _ad9371->set_fir(name, 6, fir->get_taps());
}

std::vector<std::string> magnesium_radio_control_impl::get_tx_filter_names(
    const size_t chan) const
{
    UHD_ASSERT_THROW(chan < TOTAL_RADIO_PORTS);
    if (chan % 2 == 0) {
        return {"TX1_FIR", "TX1TX2_FIR"};
    } else {
        return {"TX2_FIR", "TX1TX2_FIR"};
    }
}

uhd::filter_info_base::sptr magnesium_radio_control_impl::get_tx_filter(
    const std::string& name, const size_t)
{
    if (_mpm_compat_num[0] < 4 || (_mpm_compat_num[0] == 4 && _mpm_compat_num[1] < 2)) {
        RFNOC_LOG_WARNING("Getting tx filter not supported. Please upgrade MPM to a "
                          "minimum version of 4.2.");
        return std::make_shared<uhd::digital_filter_fir<int16_t>>(
            uhd::filter_info_base::filter_type::DIGITAL_FIR_I16,
            false,
            0,
            1.0,
            1,
            1,
            32767,
            AD9371_TX_MAX_FIR_TAPS,
            std::vector<int16_t>(AD9371_TX_MAX_FIR_TAPS, 0));
    }

    const auto rv   = _ad9371->get_fir(name);
    const auto taps = rv.second;
    // TODO: Use gain in the digital_filter_fir
    return std::make_shared<uhd::digital_filter_fir<int16_t>>(
        uhd::filter_info_base::filter_type::DIGITAL_FIR_I16,
        false,
        0,
        1.0,
        1,
        1,
        32767,
        AD9371_TX_MAX_FIR_TAPS,
        taps);
}

void magnesium_radio_control_impl::set_tx_filter(
    const std::string& name, uhd::filter_info_base::sptr filter, const size_t)
{
    std::lock_guard<std::recursive_mutex> l(_set_lock);

    if (_mpm_compat_num[0] < 4 || (_mpm_compat_num[0] == 4 && _mpm_compat_num[1] < 2)) {
        RFNOC_LOG_WARNING("Setting tx filter not supported. Please upgrade MPM to a "
                          "minimum version of 4.2.");
        return;
    }

    auto fir = std::dynamic_pointer_cast<uhd::digital_filter_fir<int16_t>>(filter);
    if (fir == nullptr) {
        throw uhd::runtime_error("Invalid Filter Type for TX Filter");
    }
    if (fir->get_taps().size() != AD9371_TX_MAX_FIR_TAPS) {
        throw uhd::runtime_error("AD937x TX Filter Taps must be "
                                 + std::to_string(AD9371_TX_MAX_FIR_TAPS)
                                 + " taps long!");
    }
    // TODO: Use gain in the digital_filter_fir
    _ad9371->set_fir(name, 6, fir->get_taps());
}

/**************************************************************************
 * Radio Identification API Calls
 *************************************************************************/
size_t magnesium_radio_control_impl::get_chan_from_dboard_fe(
    const std::string& fe, const uhd::direction_t) const
{
    if (fe == "0") {
        return 0;
    }
    if (fe == "1") {
        return 1;
    }
    throw uhd::key_error(std::string("[N300] Invalid frontend: ") + fe);
}

std::string magnesium_radio_control_impl::get_dboard_fe_from_chan(
    const size_t chan, const uhd::direction_t) const
{
    if (chan == 0) {
        return "0";
    }
    if (chan == 1) {
        return "1";
    }
    throw uhd::lookup_error(
        std::string("[N300] Invalid channel: ") + std::to_string(chan));
}

std::string magnesium_radio_control_impl::get_fe_name(
    const size_t, const uhd::direction_t) const
{
    return MAGNESIUM_FE_NAME;
}

// Register the block
UHD_RFNOC_BLOCK_REGISTER_FOR_DEVICE_DIRECT(
    magnesium_radio_control, RADIO_BLOCK, N300, "Radio", true, "radio_clk", "bus_clk");

//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/usrp/zbx_tune_map_item.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/dboard/zbx/zbx_expert.hpp>
#include <uhdlib/utils/interpolation.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <algorithm>
#include <array>

using namespace uhd;

namespace uhd { namespace usrp { namespace zbx {

namespace {

/*********************************************************************
 *   Misc/calculative helper functions
 **********************************************************************/
bool _is_band_highband(const zbx_tune_map_item_t tune_setting)
{
    // Lowband frequency paths do not utilize an RF filter
    return tune_setting.rf_fltr == 0;
}

zbx_tune_map_item_t _get_tune_settings(
    const double freq, const std::vector<zbx_tune_map_item_t>& tune_map)
{
    auto tune_setting      = tune_map.begin();
    auto tune_settings_end = tune_map.end();

    for (; tune_setting != tune_settings_end; ++tune_setting) {
        if (tune_setting->max_band_freq >= freq) {
            return *tune_setting;
        }
    }
    // Didn't find a tune setting.  This frequency should have been clipped, this is
    // an internal error.
    UHD_THROW_INVALID_CODE_PATH();
}

bool _is_band_inverted(const uhd::direction_t trx,
    const double if2_freq,
    const double rfdc_rate,
    const zbx_tune_map_item_t tune_setting)
{
    const bool is_if2_nyquist2 = if2_freq > (rfdc_rate / 2);

    // We count the number of inversions introduced by the signal chain, starting
    // at the RFDC
    const int num_inversions =
        // If we're in the second Nyquist zone, we're inverted
        int(is_if2_nyquist2) +
        // LO2 mixer may invert
        int(tune_setting.lo2_inj_side == HIGH) +
        // LO1 mixer can only invert in the lowband
        int(!_is_band_highband(tune_setting) && tune_setting.lo1_inj_side == HIGH);

    // In the RX direction, an extra inversion is needed
    // TODO: We don't know where this is coming from
    const bool num_inversions_is_odd = num_inversions % 2 != 0;
    if (trx == RX_DIRECTION) {
        return !num_inversions_is_odd;
    } else {
        return num_inversions_is_odd;
    }
}

double _calc_lo2_freq(
    const double if1_freq, const double if2_freq, const lo_inj_side_t lo2_inj_side)
{
    return if1_freq - lo2_inj_side * if2_freq;
}

double _calc_if2_freq(const double if1_freq, const double lo2_freq)
{
    return abs(if1_freq - lo2_freq);
}

std::string _get_trx_string(const direction_t dir)
{
    if (dir == RX_DIRECTION) {
        return "rx";
    } else if (dir == TX_DIRECTION) {
        return "tx";
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}

// For various RF performance considerations (such as spur reduction), different bands
// vary between using fixed IF1 and/or IF2 or using variable IF1 and/or IF2. Bands
// with a fixed IF1/IF2 have ifX_freq_min == IFX_freq_max, and _calc_ifX_freq() will
// return that single value. Bands with variable IF1/IF2 will shift the IFX based on
// where in the RF band we are tuning by using linear interpolation. (if1 calculation
// takes place only if tune frequency is lowband)
double _calc_if1_freq(const double tune_freq, const zbx_tune_map_item_t tune_setting)
{
    if (tune_setting.if1_freq_min == tune_setting.if1_freq_max) {
        return tune_setting.if1_freq_min;
    }

    return uhd::math::linear_interp(tune_freq,
        tune_setting.min_band_freq,
        tune_setting.if1_freq_min,
        tune_setting.max_band_freq,
        tune_setting.if1_freq_max);
}

double _calc_ideal_if2_freq(
    const double tune_freq, const zbx_tune_map_item_t tune_setting)
{
    // linear_interp() wants to interpolate and will throw if these are identical:
    if (tune_setting.if2_freq_min == tune_setting.if2_freq_max) {
        return tune_setting.if2_freq_min;
    }

    return uhd::math::linear_interp(tune_freq,
        tune_setting.min_band_freq,
        tune_setting.if2_freq_min,
        tune_setting.max_band_freq,
        tune_setting.if2_freq_max);
}

} // namespace

/*!---------------------------------------------------------
 * EXPERT RESOLVE FUNCTIONS
 *
 * This sections contains all expert resolve functions.
 * These methods are triggered by any of the bound accessors becoming "dirty",
 * or changing value
 * --------------------------------------------------------
 */
void zbx_scheduling_expert::resolve()
{
    // We currently have no fancy scheduling, but here is where we'd add it if
    // we need to do that (e.g., plan out SYNC pulse timing vs. NCO timing etc.)
    _frontend_time = _command_time;
}

void zbx_freq_fe_expert::resolve()
{
    const double tune_freq = ZBX_FREQ_RANGE.clip(_desired_frequency);
    _tune_settings         = _get_tune_settings(tune_freq, _tune_table.get());

    // Set mixer values so the backend expert knows how to calculate final frequency
    _lo1_inj_side = _tune_settings.lo1_inj_side;
    _lo2_inj_side = _tune_settings.lo2_inj_side;

    _is_highband = _is_band_highband(_tune_settings);
    _lo1_enabled = !_is_highband.get();

    double if1_freq      = tune_freq;
    const double lo_step = _lo_freq_range.step();
    // If we need to apply an offset to avoid injection locking, we need to
    // offset in different directions for different channels on the same zbx
    const double lo_offset_sign = (_chan == 0) ? -1 : 1;
    // In high band, LO1 is not needed (the signal is already at a high enough
    // frequency for the second stage)
    if (_lo1_enabled) {
        // Calculate the ideal IF1:
        if1_freq = _calc_if1_freq(tune_freq, _tune_settings);
        // We calculate the LO1 frequency by first shifting the tune frequency to the
        // desired IF, and then applying an offset such that CH0 and CH1 tune to distinct
        // LO1 frequencies: This is done to prevent the LO's from interfering with each
        // other in a phenomenon known as injection locking.
        const double lo1_freq = if1_freq - (_tune_settings.lo1_inj_side * tune_freq)
                                + (lo_offset_sign * lo_step);
        // Now, quantize the LO frequency to the nearest valid value:
        _desired_lo1_frequency = _lo_freq_range.clip(lo1_freq, true);
        // Because LO1 frequency probably changed during quantization, we simply
        // re-calculate the now-valid IF1 (the following equation is the same as
        // the LO1 frequency calculation, but solved for if1_freq):
        if1_freq = _desired_lo1_frequency + (_tune_settings.lo1_inj_side * tune_freq);
    }

    _lo2_enabled = true;
    // Calculate ideal IF2 frequency:
    const double if2_freq = _calc_ideal_if2_freq(tune_freq, _tune_settings);
    // Calculate LO2 frequency from that:
    _desired_lo2_frequency = _calc_lo2_freq(if1_freq, if2_freq, _lo2_inj_side);
    // Similar to LO1, apply an offset such that CH0 and CH1 tune to distinct LO2
    // frequencies to prevent potential interference between CH0 and CH1 LO2's from
    // injection locking: In highband (LO1 disabled), this must explicitly be done below.
    // In lowband (LO1 enabled), the LO1 will have already been shifted and, as a result,
    // the LO2's will have already been shifted to compensate for LO1 in previous
    // function. Note that in lowband, the LO1's and LO2's will be offset between CH0 and
    // CH1; however, they will be offset in opposite direction such that the NCO frequency
    // will be the same between CH0 and CH1. This is not the case for highband (only LO2
    // and they must be offset).
    if (!_lo1_enabled) {
        _desired_lo2_frequency = _desired_lo2_frequency + (lo_offset_sign * lo_step);
    }
    // Now, quantize the LO frequency to the nearest valid value:
    _desired_lo2_frequency = _lo_freq_range.clip(_desired_lo2_frequency, true);
    // Calculate actual IF2 frequency from LO2 and IF1 frequencies:
    _desired_if2_frequency = _calc_if2_freq(if1_freq, _desired_lo2_frequency);

    // If the frequency is in a different tuning band, we need to switch filters
    _rf_filter  = _tune_settings.rf_fltr;
    _if1_filter = _tune_settings.if1_fltr;
    _if2_filter = _tune_settings.if2_fltr;
    _band_inverted =
        _is_band_inverted(_trx, _desired_if2_frequency, _rfdc_rate, _tune_settings);
}


void zbx_freq_be_expert::resolve()
{
    const double coerced_if1_freq =
        _coerced_lo2_frequency + (_coerced_if2_frequency * _lo2_inj_side);
    if (_is_highband) {
        _coerced_frequency = coerced_if1_freq;
    } else {
        _coerced_frequency = std::abs(coerced_if1_freq - _coerced_lo1_frequency);
    }

    // Users may change individual settings (LO frequencies, if2 frequencies) and throw
    // the output frequency out of range. We have to stop here so that the gain API
    // doesn't panic (Clipping here would have no effect on the actual output signal)
    using namespace uhd::math::fp_compare;
    if (fp_compare_delta<double>(_coerced_frequency.get()) < ZBX_MIN_FREQ
        || fp_compare_delta<double>(_coerced_frequency.get()) > ZBX_MAX_FREQ) {
        UHD_LOG_WARNING(get_name(),
            "Resulting coerced frequency " << _coerced_frequency.get()
                                           << " is out of range!");
    }
}

void zbx_lo_expert::resolve()
{
    if (_test_mode_enabled.is_dirty()) {
        _lo_ctrl->set_lo_test_mode_enabled(_test_mode_enabled);
    }

    if (_set_is_enabled.is_dirty()) {
        _lo_ctrl->set_lo_port_enabled(_set_is_enabled);
    }

    if (_set_is_enabled && _desired_lo_frequency.is_dirty()) {
        const double clipped_lo_freq = std::max(
            LMX2572_MIN_FREQ, std::min(_desired_lo_frequency.get(), LMX2572_MAX_FREQ));
        _coerced_lo_frequency = _lo_ctrl->set_lo_freq(clipped_lo_freq);
    }
}

void zbx_gain_coercer_expert::resolve()
{
    _gain_coerced = _valid_range.clip(_gain_desired, true);
}

void zbx_tx_gain_expert::resolve()
{
    if (_profile != ZBX_GAIN_PROFILE_DEFAULT) {
        return;
    }

    // If a user passes in a gain value, we have to set the Power API tracking mode
    if (_gain_in.is_dirty()) {
        _power_mgr->set_tracking_mode(uhd::usrp::pwr_cal_mgr::tracking_mode::TRACK_GAIN);
    }

    // Now we do the overall gain setting
    // Look up DSA values by gain
    _gain_out             = ZBX_TX_GAIN_RANGE.clip(_gain_in, true);
    const size_t gain_idx = _gain_out / TX_GAIN_STEP;
    // Clip _frequency to valid ZBX range to avoid errors in the scenario when user
    // manually configures LO frequencies and causes an illegal overall frequency
    auto dsa_settings =
        _dsa_cal->get_dsa_setting(ZBX_FREQ_RANGE.clip(_frequency), gain_idx);
    // Now write to downstream nodes, converting attenuations to gains:
    _dsa1 = static_cast<double>(ZBX_TX_DSA_MAX_ATT - dsa_settings[0]);
    _dsa2 = static_cast<double>(ZBX_TX_DSA_MAX_ATT - dsa_settings[1]);
    // Convert amp index to gain
    _amp_gain = ZBX_TX_AMP_GAIN_MAP.at(static_cast<tx_amp>(dsa_settings[2]));
}

void zbx_rx_gain_expert::resolve()
{
    if (_profile != ZBX_GAIN_PROFILE_DEFAULT) {
        return;
    }

    // If a user passes in a gain value, we have to set the Power API tracking mode
    if (_gain_in.is_dirty()) {
        _power_mgr->set_tracking_mode(uhd::usrp::pwr_cal_mgr::tracking_mode::TRACK_GAIN);
    }

    // Now we do the overall gain setting
    if (_frequency.get() <= RX_LOW_FREQ_MAX_GAIN_CUTOFF) {
        _gain_out = ZBX_RX_LOW_FREQ_GAIN_RANGE.clip(_gain_in, true);
    } else {
        _gain_out = ZBX_RX_GAIN_RANGE.clip(_gain_in, true);
    }
    // Now we do the overall gain setting
    // Look up DSA values by gain
    const size_t gain_idx = _gain_out / RX_GAIN_STEP;
    // Clip _frequency to valid ZBX range to avoid errors in the scenario when user
    // manually configures LO frequencies and causes an illegal overall frequency
    auto dsa_settings =
        _dsa_cal->get_dsa_setting(ZBX_FREQ_RANGE.clip(_frequency), gain_idx);
    // Now write to downstream nodes, converting attenuation to gains:
    _dsa1  = ZBX_RX_DSA_MAX_ATT - dsa_settings[0];
    _dsa2  = ZBX_RX_DSA_MAX_ATT - dsa_settings[1];
    _dsa3a = ZBX_RX_DSA_MAX_ATT - dsa_settings[2];
    _dsa3b = ZBX_RX_DSA_MAX_ATT - dsa_settings[3];
}

void zbx_tx_programming_expert::resolve()
{
    if (_profile.is_dirty()) {
        if (_profile == ZBX_GAIN_PROFILE_DEFAULT || _profile == ZBX_GAIN_PROFILE_MANUAL
            || _profile == ZBX_GAIN_PROFILE_CPLD) {
            _cpld->set_atr_mode(_chan,
                zbx_cpld_ctrl::atr_mode_target::DSA,
                zbx_cpld_ctrl::atr_mode::CLASSIC_ATR);
        } else {
            _cpld->set_atr_mode(_chan,
                zbx_cpld_ctrl::atr_mode_target::DSA,
                zbx_cpld_ctrl::atr_mode::SW_DEFINED);
        }
    }

    // If we're in any of the table modes, then we don't write DSA and amp values
    // A note on caching: The CPLD object caches state, and only pokes the CPLD
    // if it's changed. However, all DSAs are on the same register. That means
    // the DSA register changes, all DSA values written to the CPLD will come
    // from the input data nodes to this worker node. This can overwrite DSA
    // values if the cached version and the actual value on the CPLD differ.
    if (_profile == ZBX_GAIN_PROFILE_DEFAULT || _profile == ZBX_GAIN_PROFILE_MANUAL) {
        // Convert gains back to attenuation
        zbx_cpld_ctrl::tx_dsa_type dsa_settings = {
            uhd::narrow_cast<uint32_t>(ZBX_TX_DSA_MAX_ATT - _dsa1.get()),
            uhd::narrow_cast<uint32_t>(ZBX_TX_DSA_MAX_ATT - _dsa2.get())};
        _cpld->set_tx_gain_switches(_chan, ATR_ADDR_TX, dsa_settings);
        _cpld->set_tx_gain_switches(_chan, ATR_ADDR_XX, dsa_settings);
    }

    // If frequency changed, we might have changed bands and the CPLD dsa tables need to
    // be reloaded
    // TODO: This is a major hack, and these tables should be loaded outside of the
    // tuning call.  This means every tuning request involves a large amount of CPLD
    // writes.
    // We only write when we aren't using a command time, otherwise all those CPLD
    // commands will line up in the CPLD command queue, and diminish any purpose
    // of timed commands in the first place
    // Clip _frequency to valid ZBX range to avoid errors in the scenario when user
    // manually configures LO frequencies and causes an illegal overall frequency
    if (_command_time == 0.0) {
        _cpld->update_tx_dsa_settings(
            _dsa_cal->get_band_settings(ZBX_FREQ_RANGE.clip(_frequency), 0 /*dsa1*/),
            _dsa_cal->get_band_settings(ZBX_FREQ_RANGE.clip(_frequency), 1 /*dsa2*/));
    }

    for (const size_t idx : ATR_ADDRS) {
        _cpld->set_lo_source(idx,
            zbx_lo_ctrl::lo_string_to_enum(TX_DIRECTION, _chan, ZBX_LO1),
            _lo1_source);
        _cpld->set_lo_source(idx,
            zbx_lo_ctrl::lo_string_to_enum(TX_DIRECTION, _chan, ZBX_LO2),
            _lo2_source);

        _cpld->set_tx_rf_filter(_chan, idx, _rf_filter);
        _cpld->set_tx_if1_filter(_chan, idx, _if1_filter);
        _cpld->set_tx_if2_filter(_chan, idx, _if2_filter);
    }

    // Convert amp gain to amp index
    UHD_ASSERT_THROW(ZBX_TX_GAIN_AMP_MAP.count(_amp_gain.get()));
    const tx_amp amp = ZBX_TX_GAIN_AMP_MAP.at(_amp_gain.get());
    _cpld->set_tx_antenna_switches(_chan, ATR_ADDR_0X, _antenna, tx_amp::BYPASS);
    _cpld->set_tx_antenna_switches(_chan, ATR_ADDR_RX, _antenna, tx_amp::BYPASS);
    _cpld->set_tx_antenna_switches(_chan, ATR_ADDR_TX, _antenna, amp);
    _cpld->set_tx_antenna_switches(_chan, ATR_ADDR_XX, _antenna, amp);

    // We do not update LEDs on switching TX antenna value by definition
}

void zbx_rx_programming_expert::resolve()
{
    if (_profile.is_dirty()) {
        if (_profile == ZBX_GAIN_PROFILE_DEFAULT || _profile == ZBX_GAIN_PROFILE_MANUAL
            || _profile == ZBX_GAIN_PROFILE_CPLD) {
            _cpld->set_atr_mode(_chan,
                zbx_cpld_ctrl::atr_mode_target::DSA,
                zbx_cpld_ctrl::atr_mode::CLASSIC_ATR);
        } else {
            _cpld->set_atr_mode(_chan,
                zbx_cpld_ctrl::atr_mode_target::DSA,
                zbx_cpld_ctrl::atr_mode::SW_DEFINED);
        }
    }

    // If we're in any of the table modes, then we don't write DSA values
    // A note on caching: The CPLD object caches state, and only pokes the CPLD
    // if it's changed. However, all DSAs are on the same register. That means
    // the DSA register changes, all DSA values written to the CPLD will come
    // from the input data nodes to this worker node. This can overwrite DSA
    // values if the cached version and the actual value on the CPLD differ.
    if (_profile == ZBX_GAIN_PROFILE_DEFAULT || _profile == ZBX_GAIN_PROFILE_MANUAL) {
        zbx_cpld_ctrl::rx_dsa_type dsa_settings = {
            uhd::narrow_cast<uint32_t>(ZBX_RX_DSA_MAX_ATT - _dsa1.get()),
            uhd::narrow_cast<uint32_t>(ZBX_RX_DSA_MAX_ATT - _dsa2.get()),
            uhd::narrow_cast<uint32_t>(ZBX_RX_DSA_MAX_ATT - _dsa3a.get()),
            uhd::narrow_cast<uint32_t>(ZBX_RX_DSA_MAX_ATT - _dsa3b.get())};
        _cpld->set_rx_gain_switches(_chan, ATR_ADDR_RX, dsa_settings);
        _cpld->set_rx_gain_switches(_chan, ATR_ADDR_XX, dsa_settings);
    }


    // If frequency changed, we might have changed bands and the CPLD dsa tables need to
    // be reloaded
    // TODO: This is a major hack, and these tables should be loaded outside of the
    // tuning call.  This means every tuning request involves a large amount of CPLD
    // writes.
    // We only write when we aren't using a command time, otherwise all those CPLD
    // commands will line up in the CPLD command queue, and diminish any purpose
    // of timed commands in the first place
    // Clip _frequency to valid ZBX range to avoid errors in the scenario when user
    // manually configures LO frequencies and causes an illegal overall frequency
    if (_command_time == 0.0) {
        _cpld->update_rx_dsa_settings(
            _dsa_cal->get_band_settings(ZBX_FREQ_RANGE.clip(_frequency), 0 /*dsa1*/),
            _dsa_cal->get_band_settings(ZBX_FREQ_RANGE.clip(_frequency), 1 /*dsa2*/),
            _dsa_cal->get_band_settings(ZBX_FREQ_RANGE.clip(_frequency), 2 /*dsa3a*/),
            _dsa_cal->get_band_settings(ZBX_FREQ_RANGE.clip(_frequency), 3 /*dsa3b*/));
    }

    for (const size_t idx : ATR_ADDRS) {
        _cpld->set_lo_source(idx,
            zbx_lo_ctrl::lo_string_to_enum(RX_DIRECTION, _chan, ZBX_LO1),
            _lo1_source);
        _cpld->set_lo_source(idx,
            zbx_lo_ctrl::lo_string_to_enum(RX_DIRECTION, _chan, ZBX_LO2),
            _lo2_source);

        // If using the TX/RX terminal, only configure the ATR RX state since the
        // state of the switch at other times is controlled by TX
        if (_antenna != ANTENNA_TXRX || idx == ATR_ADDR_RX) {
            _cpld->set_rx_antenna_switches(_chan, idx, _antenna);
        }

        _cpld->set_rx_rf_filter(_chan, idx, _rf_filter);
        _cpld->set_rx_if1_filter(_chan, idx, _if1_filter);
        _cpld->set_rx_if2_filter(_chan, idx, _if2_filter);
    }

    _update_leds();
}

void zbx_rx_programming_expert::_update_leds()
{
    if (_atr_mode != zbx_cpld_ctrl::atr_mode::CLASSIC_ATR) {
        return;
    }
    // We default to the RX1 LED for all RX antenna values that are not TX/RX0
    const bool rx_on_trx = _antenna == ANTENNA_TXRX;
    // clang-format off
    // G==Green, R==Red                RX2         TX/RX-G    TX/RX-R
    _cpld->set_leds(_chan, ATR_ADDR_0X, false,      false,     false);
    _cpld->set_leds(_chan, ATR_ADDR_RX, !rx_on_trx, rx_on_trx, false);
    _cpld->set_leds(_chan, ATR_ADDR_TX, false,      false,     true );
    _cpld->set_leds(_chan, ATR_ADDR_XX, !rx_on_trx, rx_on_trx, true );
    // clang-format on
}

void zbx_band_inversion_expert::resolve()
{
    _rpcc->enable_iq_swap(_is_band_inverted.get(), _get_trx_string(_trx), _chan);
}

void zbx_rfdc_freq_expert::resolve()
{
    // Because we can configure both IF2 and the RFDC NCO frequency, these may
    // come into conflict. We choose IF2 over RFDC in that case. In other words
    // the only time we choose the desired RFDC frequency over the IF2 (when in
    // conflict) is when the RFDC freq was changed directly.
    const double desired_rfdc_freq = [&]() -> double {
        if (_rfdc_freq_desired.is_dirty() && !_if2_frequency_desired.is_dirty()) {
            return _rfdc_freq_desired;
        }
        return _if2_frequency_desired;
    }();

    _rfdc_freq_coerced = _rpcc->rfdc_set_nco_freq(
        _get_trx_string(_trx), _db_idx, _chan, desired_rfdc_freq);
    _if2_frequency_coerced = _rfdc_freq_coerced;
}

void zbx_sync_expert::resolve()
{
    // Some local helper consts
    // clang-format off
    constexpr std::array<std::array<zbx_lo_t, 4>, 2> los{{{
        zbx_lo_t::RX0_LO1,
        zbx_lo_t::RX0_LO2,
        zbx_lo_t::TX0_LO1,
        zbx_lo_t::TX0_LO2
    }, {
        zbx_lo_t::RX1_LO1,
        zbx_lo_t::RX1_LO2,
        zbx_lo_t::TX1_LO1,
        zbx_lo_t::TX1_LO2
    }}};
    constexpr std::array<std::array<rfdc_control::rfdc_type, 2>, 2> ncos{{
        {rfdc_control::rfdc_type::RX0, rfdc_control::rfdc_type::TX0},
        {rfdc_control::rfdc_type::RX1, rfdc_control::rfdc_type::TX1}
    }};
    // clang-format on

    // Now do some timing checks
    const std::vector<bool> chan_needs_sync = {_fe_time.at(0) != uhd::time_spec_t::ASAP,
        _fe_time.at(1) != uhd::time_spec_t::ASAP};
    // If there's no command time, no need to synchronize anything
    if (!chan_needs_sync[0] && !chan_needs_sync[1]) {
        UHD_LOG_TRACE(get_name(), "No command time: Skipping phase sync.");
        return;
    }
    const bool times_match = _fe_time.at(0) == _fe_time.at(1);

    // ** Find LOs to synchronize *********************************************
    // Find dirty LOs which need sync'ing
    std::set<zbx_lo_t> los_to_sync;
    for (const size_t chan : ZBX_CHANNELS) {
        if (chan_needs_sync[chan]) {
            for (const auto& lo_idx : los[chan]) {
                if (_lo_freqs.at(lo_idx).is_dirty()) {
                    los_to_sync.insert(lo_idx);
                }
            }
        }
    }

    // ** Find NCOs to synchronize ********************************************
    // Same rules apply as for LOs.
    std::set<rfdc_control::rfdc_type> ncos_to_sync;
    for (const size_t chan : ZBX_CHANNELS) {
        if (chan_needs_sync[chan]) {
            for (const auto& nco_idx : ncos[chan]) {
                if (_nco_freqs.at(nco_idx).is_dirty()) {
                    ncos_to_sync.insert(nco_idx);
                }
            }
        }
    }

    // ** Find ADC/DAC gearboxes to synchronize *******************************
    // Gearboxes are special, because they only need to be synchronized once
    // per session, assuming the command time has been set. Unfortunately we
    // have no way here to know if the timekeeper time was updated, but it is
    // well documented that in order to synchronize devices, one first has to
    // make sure the timekeepers are running in sync (by calling
    // set_time_next_pps() accordingly).
    // The logic we use here is that we will always have to update the NCO when
    // doing a synced tune, so we update all the gearboxes for the NCOs -- but
    // only if they have not yet been synchronized.
    std::set<rfdc_control::rfdc_type> gearboxes_to_sync;
    if (!_adcs_synced) {
        for (const auto rfdc :
            {rfdc_control::rfdc_type::RX0, rfdc_control::rfdc_type::RX1}) {
            if (ncos_to_sync.count(rfdc)) {
                gearboxes_to_sync.insert(rfdc);
                // Technically, they're not synced yet but this saves us from
                // having to look up which RFDCs map to RX again later
                _adcs_synced = true;
            }
        }
    }
    if (!_dacs_synced) {
        for (const auto rfdc :
            {rfdc_control::rfdc_type::TX0, rfdc_control::rfdc_type::TX1}) {
            if (ncos_to_sync.count(rfdc)) {
                gearboxes_to_sync.insert(rfdc);
                // Technically, they're not synced yet but this saves us from
                // having to look up which RFDCs map to TX again later
                _dacs_synced = true;
            }
        }
    }

    // ** Do synchronization **************************************************
    // This is where we orchestrate the sync commands. If sync commands happen
    // at different times, we make sure to send out the earlier one first.
    // If we need to schedule things a bit differently, e.g., we need to
    // manually calculate offsets from the command time so that LO and NCO sync
    // pulses line up, it most likely makes sense to use the scheduling expert
    // for that, and calculate different times for different events there.
    if (times_match) {
        UHD_LOG_TRACE(get_name(),
            "Syncing all channels: " << los_to_sync.size() << " LO(s), "
                                     << ncos_to_sync.size() << " NCO(s), and "
                                     << gearboxes_to_sync.size() << " gearbox(es).")
        if (!gearboxes_to_sync.empty()) {
            _rfdcc->reset_gearboxes(
                std::vector<rfdc_control::rfdc_type>(
                    gearboxes_to_sync.cbegin(), gearboxes_to_sync.cend()),
                _fe_time.at(0).get());
        }
        if (!los_to_sync.empty()) {
            _cpld->pulse_lo_sync(
                0, std::vector<zbx_lo_t>(los_to_sync.cbegin(), los_to_sync.cend()));
        }
        if (!ncos_to_sync.empty()) {
            _rfdcc->reset_ncos(std::vector<rfdc_control::rfdc_type>(
                                   ncos_to_sync.cbegin(), ncos_to_sync.cend()),
                _fe_time.at(0).get());
        }
    } else {
        // If the command times differ, we need to manually reorder the commands
        // such that the channel with the earlier time gets precedence
        const size_t first_sync_chan =
            (times_match || (_fe_time.at(0) <= _fe_time.at(1))) ? 0 : 1;
        const auto sync_order = (first_sync_chan == 0) ? std::vector<size_t>{0, 1}
                                                       : std::vector<size_t>{1, 0};
        for (const size_t chan : sync_order) {
            std::vector<zbx_lo_t> this_chan_los;
            for (const zbx_lo_t lo_idx : los[chan]) {
                if (los_to_sync.count(lo_idx)) {
                    this_chan_los.push_back(lo_idx);
                }
            }

            std::vector<rfdc_control::rfdc_type> this_chan_ncos;
            for (const auto nco_idx : ncos[chan]) {
                if (ncos_to_sync.count(nco_idx)) {
                    this_chan_ncos.push_back(nco_idx);
                }
            }
            std::vector<rfdc_control::rfdc_type> this_chan_gearboxes;
            for (const auto gb_idx : ncos[chan]) {
                if (gearboxes_to_sync.count(gb_idx)) {
                    this_chan_gearboxes.push_back(gb_idx);
                }
            }
            UHD_LOG_TRACE(get_name(),
                "Syncing channel " << chan << ": " << this_chan_los.size()
                                   << " LO(s) and " << this_chan_ncos.size()
                                   << " NCO(s).");
            if (!this_chan_gearboxes.empty()) {
                UHD_LOG_TRACE(get_name(),
                    "Resetting " << this_chan_gearboxes.size() << " gearboxes.");
                _rfdcc->reset_gearboxes(this_chan_gearboxes, _fe_time.at(chan).get());
            }
            if (!this_chan_los.empty()) {
                _cpld->pulse_lo_sync(chan, this_chan_los);
            }
            if (!this_chan_ncos.empty()) {
                _rfdcc->reset_ncos(this_chan_ncos, _fe_time.at(chan).get());
            }
        }
    }
} // zbx_sync_expert::resolve()

// End expert resolve sections

}}} // namespace uhd::usrp::zbx

//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "twinrx_experts.hpp"
#include "twinrx_gain_tables.hpp"
#include <uhd/utils/math.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/dict.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace uhd::experts;
using namespace uhd::math;
using namespace uhd::usrp::dboard::twinrx;

/*!---------------------------------------------------------
 * twinrx_scheduling_expert::resolve
 * ---------------------------------------------------------
 */
void twinrx_scheduling_expert::resolve()
{
    // Currently a straight pass-through. To be expanded as needed
    // when more advanced scheduling is needed
    _rx_frontend_time = _command_time;
}

/*!---------------------------------------------------------
 * twinrx_freq_path_expert::resolve
 * ---------------------------------------------------------
 */
void twinrx_freq_path_expert::resolve()
{
    //Lowband/highband switch point
    static const double LB_HB_THRESHOLD_FREQ    = 1.8e9;
    static const double LB_TARGET_IF1_FREQ      = 2.345e9;
    static const double HB_TARGET_IF1_FREQ      = 1.25e9;
    static const double INJ_SIDE_THRESHOLD_FREQ = 5.1e9;

    static const double FIXED_LO1_THRESHOLD_FREQ= 50e6;

    //Preselector filter switch point
    static const double LB_FILT1_THRESHOLD_FREQ = 0.5e9;
    static const double LB_FILT2_THRESHOLD_FREQ = 0.8e9;
    static const double LB_FILT3_THRESHOLD_FREQ = 1.2e9;
    static const double LB_FILT4_THRESHOLD_FREQ = 1.8e9;
    static const double HB_FILT1_THRESHOLD_FREQ = 3.0e9;
    static const double HB_FILT2_THRESHOLD_FREQ = 4.1e9;
    static const double HB_FILT3_THRESHOLD_FREQ = 5.1e9;
    static const double HB_FILT4_THRESHOLD_FREQ = 6.0e9;

    static const double LB_PREAMP_PRESEL_THRESHOLD_FREQ = 0.8e9;

    //Misc
    static const double INST_BANDWIDTH           = 80e6;
    static const double MANUAL_LO_HYSTERESIS_PPM = 1.0;

    static const freq_range_t FREQ_RANGE(10e6, 6e9);
    rf_freq_abs_t rf_freq(FREQ_RANGE.clip(_rf_freq_d));

    // Choose low-band vs high-band depending on frequency
    _signal_path = (rf_freq > LB_HB_THRESHOLD_FREQ) ?
        twinrx_ctrl::PATH_HIGHBAND : twinrx_ctrl::PATH_LOWBAND;
    if (_signal_path == twinrx_ctrl::PATH_LOWBAND) {
        // Choose low-band preselector filter
        if (rf_freq < LB_FILT1_THRESHOLD_FREQ) {
            _lb_presel = twinrx_ctrl::PRESEL_PATH1;
        } else if (rf_freq < LB_FILT2_THRESHOLD_FREQ) {
            _lb_presel = twinrx_ctrl::PRESEL_PATH2;
        } else if (rf_freq < LB_FILT3_THRESHOLD_FREQ) {
            _lb_presel = twinrx_ctrl::PRESEL_PATH3;
        } else if (rf_freq < LB_FILT4_THRESHOLD_FREQ) {
            _lb_presel = twinrx_ctrl::PRESEL_PATH4;
        } else {
            _lb_presel = twinrx_ctrl::PRESEL_PATH4;
        }
    } else if (_signal_path == twinrx_ctrl::PATH_HIGHBAND) {
        // Choose high-band preselector filter
        if (rf_freq < HB_FILT1_THRESHOLD_FREQ) {
            _hb_presel = twinrx_ctrl::PRESEL_PATH1;
        } else if (rf_freq < HB_FILT2_THRESHOLD_FREQ) {
            _hb_presel = twinrx_ctrl::PRESEL_PATH2;
        } else if (rf_freq < HB_FILT3_THRESHOLD_FREQ) {
            _hb_presel = twinrx_ctrl::PRESEL_PATH3;
        } else if (rf_freq < HB_FILT4_THRESHOLD_FREQ) {
            _hb_presel = twinrx_ctrl::PRESEL_PATH4;
        } else {
            _hb_presel = twinrx_ctrl::PRESEL_PATH4;
        }
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }

    //Choose low-band preamp preselector
    _lb_preamp_presel = (rf_freq > LB_PREAMP_PRESEL_THRESHOLD_FREQ);

    //Choose LO frequencies
    const double target_if1_freq = (_signal_path == twinrx_ctrl::PATH_HIGHBAND) ?
        HB_TARGET_IF1_FREQ : LB_TARGET_IF1_FREQ;
    const double target_if2_freq = _if_freq_d;

    // LO1
    double lo1_freq_ideal = 0.0, lo2_freq_ideal = 0.0;
    if (rf_freq <= FIXED_LO1_THRESHOLD_FREQ) {
        //LO1 Freq static
        lo1_freq_ideal = target_if1_freq + FIXED_LO1_THRESHOLD_FREQ;
    } else if (rf_freq <= INJ_SIDE_THRESHOLD_FREQ) {
        //High-side LO1 Injection
        lo1_freq_ideal = rf_freq.get() + target_if1_freq;
    } else {
        //Low-side LO1 Injection
        lo1_freq_ideal = rf_freq.get() - target_if1_freq;
    }

    if (_lo1_freq_d.get_author() == experts::AUTHOR_USER) {
        if (_lo1_freq_d.is_dirty()) { //Are we here because the LO frequency was set?
            // The user explicitly requested to set the LO freq so don't touch it!
        } else {
            // Something else changed which may cause the LO frequency to update.
            // Only commit if the frequency is stale. If the user's value is stale
            // reset the author to expert.
            if (rf_freq_ppm_t(lo1_freq_ideal, MANUAL_LO_HYSTERESIS_PPM) != _lo1_freq_d.get()) {
                _lo1_freq_d = lo1_freq_ideal;    //Reset author
            }
        }
    } else {
        // The LO frequency was never set by the user. Let the expert take care of it
        _lo1_freq_d = lo1_freq_ideal;    //Reset author
    }

    // LO2
    lo_inj_side_t lo2_inj_side_ideal = _compute_lo2_inj_side(
        lo1_freq_ideal, target_if1_freq, target_if2_freq, INST_BANDWIDTH);
    if (lo2_inj_side_ideal == INJ_HIGH_SIDE) {
        lo2_freq_ideal = target_if1_freq + target_if2_freq;
    } else {
        lo2_freq_ideal = target_if1_freq - target_if2_freq;
    }

    if (_lo2_freq_d.get_author() == experts::AUTHOR_USER) {
        if (_lo2_freq_d.is_dirty()) { //Are we here because the LO frequency was set?
            // The user explicitly requested to set the LO freq so don't touch it!
        } else {
            // Something else changed which may cause the LO frequency to update.
            // Only commit if the frequency is stale. If the user's value is stale
            // reset the author to expert.
            if (rf_freq_ppm_t(lo2_freq_ideal, MANUAL_LO_HYSTERESIS_PPM) != _lo2_freq_d.get()) {
                _lo2_freq_d = lo2_freq_ideal;    //Reset author
            }
        }
    } else {
        // The LO frequency was never set by the user. Let the expert take care of it
        _lo2_freq_d = lo2_freq_ideal;    //Reset author
    }

    // Determine injection side using the final LO frequency
    _lo1_inj_side = (_lo1_freq_d > rf_freq.get())   ? INJ_HIGH_SIDE : INJ_LOW_SIDE;
    _lo2_inj_side = (_lo2_freq_d > target_if1_freq) ? INJ_HIGH_SIDE : INJ_LOW_SIDE;
}

lo_inj_side_t twinrx_freq_path_expert::_compute_lo2_inj_side(
    double lo1_freq, double if1_freq,  double if2_freq, double bandwidth
) {
    static const int MAX_SPUR_ORDER = 5;
    for (int ord = MAX_SPUR_ORDER; ord >= 1; ord--) {
        // Check high-side injection first
        if (not _has_mixer_spurs(lo1_freq, if1_freq + if2_freq, if2_freq, bandwidth, ord)) {
            return INJ_HIGH_SIDE;
        }
        // Check low-side injection second
        if (not _has_mixer_spurs(lo1_freq, if1_freq - if2_freq, if2_freq, bandwidth, ord)) {
            return INJ_LOW_SIDE;
        }
    }
    // If we reached here, then there are spurs everywhere. Pick high-side as the default
    return INJ_HIGH_SIDE;
}

bool twinrx_freq_path_expert::_has_mixer_spurs(
    double lo1_freq, double lo2_freq, double if2_freq,
    double bandwidth, int spur_order
) {
    // Iterate through all N-th order harmomic combinations
    // of LOs...
    for (int lo1h_i = 1; lo1h_i <= spur_order; lo1h_i++) {
        double lo1harm_freq = lo1_freq * lo1h_i;
        for (int lo2h_i = 1; lo2h_i <= spur_order; lo2h_i++) {
            double lo2harm_freq = lo2_freq * lo2h_i;
            double hdelta = lo1harm_freq - lo2harm_freq;
            // .. and check if there is a mixer spur in the IF band
            if (std::abs(hdelta + if2_freq) < bandwidth/2 or
                std::abs(hdelta - if2_freq) < bandwidth/2) {
                return true;
            }
        }
    }
    // No spurs were found after NxN search
    return false;
}

/*!---------------------------------------------------------
 * twinrx_freq_coercion_expert::resolve
 * ---------------------------------------------------------
 */
void twinrx_freq_coercion_expert::resolve()
{
    const double actual_if2_freq = _if_freq_d;
    const double actual_if1_freq = (_lo2_inj_side == INJ_LOW_SIDE) ?
        (_lo2_freq_c + actual_if2_freq) : (_lo2_freq_c - actual_if2_freq);

    _rf_freq_c = (_lo1_inj_side == INJ_LOW_SIDE) ?
        (_lo1_freq_c + actual_if1_freq) : (_lo1_freq_c - actual_if1_freq);
}

/*!---------------------------------------------------------
 * twinrx_nyquist_expert::resolve
 * ---------------------------------------------------------
 */
void twinrx_nyquist_expert::resolve()
{
    // Do not execute when clear_command_time is called.
    // This is a transition of the command time from non-zero to zero.
    if (_rx_frontend_time == time_spec_t(0.0) and _cached_cmd_time != time_spec_t(0.0)) {
        _cached_cmd_time = _rx_frontend_time;
        return;
    }

    // Do not execute twice for the same command time unless untimed
    if (_rx_frontend_time == _cached_cmd_time and _rx_frontend_time != time_spec_t(0.0)) {
        return;
    }
    _cached_cmd_time = _rx_frontend_time;

    double if_freq_sign = 1.0;
    if (_lo1_inj_side == INJ_HIGH_SIDE) if_freq_sign *= -1.0;
    if (_lo2_inj_side == INJ_HIGH_SIDE) if_freq_sign *= -1.0;
    _if_freq_c = _if_freq_d * if_freq_sign;

    _db_iface->set_fe_connection(dboard_iface::UNIT_RX, _channel,
        usrp::fe_connection_t(_codec_conn, _if_freq_c));
}

/*!---------------------------------------------------------
 * twinrx_chan_gain_expert::resolve
 * ---------------------------------------------------------
 */
void twinrx_chan_gain_expert::resolve()
{
    if (_gain_profile != "default") {
        //TODO: Implement me!
        throw uhd::not_implemented_error("custom gain strategies not implemeted yet");
    }

    //Lookup table using settings
    const twinrx_gain_table table = twinrx_gain_table::lookup_table(
        _signal_path,
        (_signal_path==twinrx_ctrl::PATH_HIGHBAND) ? _hb_presel : _lb_presel,
        _gain_profile);

    //Compute minimum gain. The user-specified gain value will be interpreted as
    //the gain applied on top of the minimum gain state.
    //If antennas are shared or swapped, the switch has 6dB of loss
    size_t gain_index = std::min(static_cast<size_t>(boost::math::round(_gain.get())), table.get_num_entries()-1);

    //Translate gain to an index in the gain table
    const twinrx_gain_config_t& config = table.find_by_index(gain_index);

    _input_atten = config.atten1;
    if (_signal_path == twinrx_ctrl::PATH_HIGHBAND) {
        _hb_atten = config.atten2;
    } else {
        _lb_atten = config.atten2;
    }

    // Preamp 1 should use the Highband amp for frequencies above 3 GHz
    if (_signal_path == twinrx_ctrl::PATH_HIGHBAND && _hb_presel != twinrx_ctrl::PRESEL_PATH1) {
        _preamp1 = config.amp1 ? twinrx_ctrl::PREAMP_HIGHBAND : twinrx_ctrl::PREAMP_BYPASS;
    } else {
        _preamp1 = config.amp1 ? twinrx_ctrl::PREAMP_LOWBAND : twinrx_ctrl::PREAMP_BYPASS;
    }

    _preamp2 = config.amp2;
}

/*!---------------------------------------------------------
 * twinrx_lo_config_expert::resolve
 * ---------------------------------------------------------
 */
void twinrx_lo_config_expert::resolve()
{
    static const uhd::dict<std::string, twinrx_ctrl::lo_source_t> src_lookup =
        boost::assign::map_list_of
        ("internal",  twinrx_ctrl::LO_INTERNAL)
        ("external",  twinrx_ctrl::LO_EXTERNAL)
        ("companion", twinrx_ctrl::LO_COMPANION)
        ("disabled",  twinrx_ctrl::LO_DISABLED)
        ("reimport",  twinrx_ctrl::LO_REIMPORT);

    if (src_lookup.has_key(_lo_source_ch0)) {
        _lo1_src_ch0 = _lo2_src_ch0 = src_lookup[_lo_source_ch0];
    } else {
        throw uhd::value_error("Invalid LO source for channel 0.Choose from {internal, external, companion, reimport}");
    }
    if (src_lookup.has_key(_lo_source_ch1)) {
        _lo1_src_ch1 = _lo2_src_ch1 = src_lookup[_lo_source_ch1];
    } else {
        throw uhd::value_error("Invalid LO source for channel 1.Choose from {internal, external, companion, reimport}");
    }

    twinrx_ctrl::lo_export_source_t export_src = twinrx_ctrl::LO_EXPORT_DISABLED;
    if (_lo_export_ch0 and (_lo_source_ch0 == "external")) {
        throw uhd::value_error("Cannot export an external LO for channel 0");
    }
    if (_lo_export_ch1 and (_lo_source_ch1 == "external")) {
        throw uhd::value_error("Cannot export an external LO for channel 1");
    }

    // Determine which channel will provide the exported LO
    if (_lo_export_ch0 and _lo_export_ch1) {
        throw uhd::value_error("Cannot export LOs for both channels");
    } else if (_lo_export_ch0) {
        export_src = (_lo1_src_ch0 == twinrx_ctrl::LO_COMPANION) ?
            twinrx_ctrl::LO_CH2_SYNTH : twinrx_ctrl::LO_CH1_SYNTH;
    } else if (_lo_export_ch1) {
        export_src = (_lo1_src_ch1 == twinrx_ctrl::LO_COMPANION) ?
            twinrx_ctrl::LO_CH1_SYNTH : twinrx_ctrl::LO_CH2_SYNTH;
    }
    _lo1_export_src = _lo2_export_src = export_src;
}

/*!---------------------------------------------------------
 * twinrx_lo_freq_expert::resolve
 * ---------------------------------------------------------
 */
void twinrx_lo_mapping_expert::resolve()
{
    static const size_t CH0_MSK = 0x1;
    static const size_t CH1_MSK = 0x2;

    // Determine which channels are "driving" each synthesizer
    // First check for explicit requests
    // "internal" or "reimport" -> this channel
    // "companion" -> other channel
    size_t synth_map[] = {0, 0};
    if (_lox_src_ch0 == twinrx_ctrl::LO_INTERNAL or _lox_src_ch0 == twinrx_ctrl::LO_REIMPORT) {
        synth_map[0] = synth_map[0] | CH0_MSK;
    } else if (_lox_src_ch0 == twinrx_ctrl::LO_COMPANION) {
        synth_map[1] = synth_map[1] | CH0_MSK;
    }
    if (_lox_src_ch1 == twinrx_ctrl::LO_INTERNAL or _lox_src_ch1 == twinrx_ctrl::LO_REIMPORT) {
        synth_map[1] = synth_map[1] | CH1_MSK;
    } else if (_lox_src_ch1 == twinrx_ctrl::LO_COMPANION) {
        synth_map[0] = synth_map[0] | CH1_MSK;
    }

    // If a particular channel has its LO source disabled then the other
    // channel is automatically put in hop mode i.e. the synthesizer that
    // belongs to the disabled channel can be re-purposed as a redundant LO
    // to overlap tuning with signal dwell time.
    bool hopping_enabled = false;
    if (_lox_src_ch0 == twinrx_ctrl::LO_DISABLED) {
        if (_lox_src_ch1 == twinrx_ctrl::LO_INTERNAL or _lox_src_ch1 == twinrx_ctrl::LO_REIMPORT) {
            synth_map[0] = synth_map[0] | CH0_MSK;
            hopping_enabled = true;
        } else if (_lox_src_ch1 == twinrx_ctrl::LO_COMPANION) {
            synth_map[1] = synth_map[1] | CH0_MSK;
            hopping_enabled = true;
        }
    }
    if (_lox_src_ch1 == twinrx_ctrl::LO_DISABLED) {
        if (_lox_src_ch0 == twinrx_ctrl::LO_INTERNAL or _lox_src_ch0 == twinrx_ctrl::LO_REIMPORT) {
            synth_map[1] = synth_map[1] | CH1_MSK;
            hopping_enabled = true;
        } else if (_lox_src_ch0 == twinrx_ctrl::LO_COMPANION) {
            synth_map[0] = synth_map[0] | CH1_MSK;
            hopping_enabled = true;
        }
    }

    // For each synthesizer come up with the final mapping
    for (size_t synth = 0; synth < 2; synth++) {
        experts::data_writer_t<lo_synth_mapping_t>& lox_mapping =
            (synth == 0) ? _lox_mapping_synth0 : _lox_mapping_synth1;
        if (synth_map[synth] == (CH0_MSK|CH1_MSK)) {
            lox_mapping = MAPPING_SHARED;
        } else if (synth_map[synth] == CH0_MSK) {
            lox_mapping = MAPPING_CH0;
        } else if (synth_map[synth] == CH1_MSK) {
            lox_mapping = MAPPING_CH1;
        } else {
            lox_mapping = MAPPING_NONE;
        }
    }
    _lox_hopping_enabled = hopping_enabled;
}

/*!---------------------------------------------------------
 * twinrx_antenna_expert::resolve
 * ---------------------------------------------------------
 */
void twinrx_antenna_expert::resolve()
{
    static const std::string ANT0 = "RX1", ANT1 = "RX2";

    if (_antenna_ch0 == ANT0 and _antenna_ch1 == ANT1) {
        _ant_mapping = twinrx_ctrl::ANTX_NATIVE;
    } else if (_antenna_ch0 == ANT0 and _antenna_ch1 == ANT0) {
        if (_enabled_ch0 and _enabled_ch1) {
            _ant_mapping = twinrx_ctrl::ANT1_SHARED;
        } else if (_enabled_ch0) {
            _ant_mapping = twinrx_ctrl::ANTX_NATIVE;
        } else if (_enabled_ch1) {
            _ant_mapping = twinrx_ctrl::ANTX_SWAPPED;
        }
    } else if (_antenna_ch0 == ANT1 and _antenna_ch1 == ANT1) {
        if (_enabled_ch0 and _enabled_ch1) {
            _ant_mapping = twinrx_ctrl::ANT2_SHARED;
        } else if (_enabled_ch0) {
            _ant_mapping = twinrx_ctrl::ANTX_SWAPPED;
        } else if (_enabled_ch1) {
            _ant_mapping = twinrx_ctrl::ANTX_NATIVE;
        }
    } else if (_antenna_ch0 == ANT1 and _antenna_ch1 == ANT0) {
        _ant_mapping = twinrx_ctrl::ANTX_SWAPPED;
    } else if (_antenna_ch0 != ANT0 and _antenna_ch0 != ANT1) {
        throw uhd::value_error("Invalid antenna selection " + _antenna_ch0.get() + " for channel 0. Must be " + ANT0 + " or " + ANT1);
    } else if (_antenna_ch1 != ANT0 and _antenna_ch1 != ANT1) {
        throw uhd::value_error("Invalid antenna selection " + _antenna_ch1.get() + " for channel 1. Must be " + ANT0 + " or " + ANT1);
    }

    //TODO: Implement hooks for the calibration switch
    _cal_mode = twinrx_ctrl::CAL_DISABLED;

    if (_cal_mode == twinrx_ctrl::CAL_CH1 and _lo_export_ch1) {
        throw uhd::value_error("Cannot calibrate channel 0 and export the LO for channel 1.");
    } else if (_cal_mode == twinrx_ctrl::CAL_CH2 and _lo_export_ch0) {
        throw uhd::value_error("Cannot calibrate channel 1 and export the LO for channel 0.");
    }
}

/*!---------------------------------------------------------
 * twinrx_ant_gain_expert::resolve
 * ---------------------------------------------------------
 */
void twinrx_ant_gain_expert::resolve()
{
    switch (_ant_mapping) {
    case twinrx_ctrl::ANTX_NATIVE:
        _ant0_input_atten       = _ch0_input_atten;
        _ant0_preamp1           = _ch0_preamp1;
        _ant0_preamp2           = _ch0_preamp2;
        _ant0_lb_preamp_presel  = _ch0_lb_preamp_presel;
        _ant1_input_atten       = _ch1_input_atten;
        _ant1_preamp1           = _ch1_preamp1;
        _ant1_preamp2           = _ch1_preamp2;
        _ant1_lb_preamp_presel  = _ch1_lb_preamp_presel;
        break;
    case twinrx_ctrl::ANTX_SWAPPED:
        _ant0_input_atten       = _ch1_input_atten;
        _ant0_preamp1           = _ch1_preamp1;
        _ant0_preamp2           = _ch1_preamp2;
        _ant0_lb_preamp_presel  = _ch1_lb_preamp_presel;
        _ant1_input_atten       = _ch0_input_atten;
        _ant1_preamp1           = _ch0_preamp1;
        _ant1_preamp2           = _ch0_preamp2;
        _ant1_lb_preamp_presel  = _ch0_lb_preamp_presel;
        break;
    case twinrx_ctrl::ANT1_SHARED:
        if ((_ch0_input_atten != _ch1_input_atten) or
            (_ch0_preamp1 != _ch1_preamp1) or
            (_ch0_preamp2 != _ch1_preamp2) or
            (_ch0_lb_preamp_presel != _ch1_lb_preamp_presel))
        {
            UHD_LOGGER_WARNING("TWINRX") << "incompatible gain settings for antenna sharing. temporarily using Ch0 settings for Ch1.";
        }
        _ant0_input_atten       = _ch0_input_atten;
        _ant0_preamp1           = _ch0_preamp1;
        _ant0_preamp2           = _ch0_preamp2;
        _ant0_lb_preamp_presel  = _ch0_lb_preamp_presel;

        _ant1_input_atten       = 0;
        _ant1_preamp1           = twinrx_ctrl::PREAMP_BYPASS;
        _ant1_preamp2           = false;
        _ant1_lb_preamp_presel  = false;
        break;
    case twinrx_ctrl::ANT2_SHARED:
        if ((_ch0_input_atten != _ch1_input_atten) or
            (_ch0_preamp1 != _ch1_preamp1) or
            (_ch0_preamp2 != _ch1_preamp2) or
            (_ch0_lb_preamp_presel != _ch1_lb_preamp_presel))
        {
            UHD_LOGGER_WARNING("TWINRX") << "incompatible gain settings for antenna sharing. temporarily using Ch0 settings for Ch1.";
        }
        _ant1_input_atten       = _ch0_input_atten;
        _ant1_preamp1           = _ch0_preamp1;
        _ant1_preamp2           = _ch0_preamp2;
        _ant1_lb_preamp_presel  = _ch0_lb_preamp_presel;

        _ant0_input_atten       = 0;
        _ant0_preamp1           = twinrx_ctrl::PREAMP_BYPASS;
        _ant0_preamp2           = false;
        _ant0_lb_preamp_presel  = false;
        break;
    default:
        _ant0_input_atten       = 0;
        _ant0_preamp1           = twinrx_ctrl::PREAMP_BYPASS;
        _ant0_preamp2           = false;
        _ant0_lb_preamp_presel  = false;
        _ant1_input_atten       = 0;
        _ant1_preamp1           = twinrx_ctrl::PREAMP_BYPASS;
        _ant1_preamp2           = false;
        _ant1_lb_preamp_presel  = false;
        break;
    }
}

/*!---------------------------------------------------------
 * twinrx_settings_expert::resolve
 * ---------------------------------------------------------
 */
const bool twinrx_settings_expert::FORCE_COMMIT = false;

void twinrx_settings_expert::resolve()
{
    for (size_t i = 0; i < 2; i++) {
        ch_settings& ch_set = (i == 1) ? _ch1 : _ch0;
        twinrx_ctrl::channel_t ch = (i == 1) ? twinrx_ctrl::CH2 : twinrx_ctrl::CH1;
        _ctrl->set_chan_enabled(ch, ch_set.chan_enabled, FORCE_COMMIT);
        _ctrl->set_preamp1(ch, ch_set.preamp1, FORCE_COMMIT);
        _ctrl->set_preamp2(ch, ch_set.preamp2, FORCE_COMMIT);
        _ctrl->set_lb_preamp_preselector(ch, ch_set.lb_preamp_presel, FORCE_COMMIT);
        _ctrl->set_signal_path(ch, ch_set.signal_path, FORCE_COMMIT);
        _ctrl->set_lb_preselector(ch, ch_set.lb_presel, FORCE_COMMIT);
        _ctrl->set_hb_preselector(ch, ch_set.hb_presel, FORCE_COMMIT);
        _ctrl->set_input_atten(ch, ch_set.input_atten, FORCE_COMMIT);
        _ctrl->set_lb_atten(ch, ch_set.lb_atten, FORCE_COMMIT);
        _ctrl->set_hb_atten(ch, ch_set.hb_atten, FORCE_COMMIT);
        _ctrl->set_lo1_source(ch, ch_set.lo1_source, FORCE_COMMIT);
        _ctrl->set_lo2_source(ch, ch_set.lo2_source, FORCE_COMMIT);
    }

    _resolve_lox_freq(STAGE_LO1,
        _ch0.lo1_freq_d, _ch1.lo1_freq_d, _ch0.lo1_freq_c, _ch1.lo1_freq_c,
        _ch0.lo1_source, _ch1.lo1_source, _lo1_synth0_mapping, _lo1_synth1_mapping,
        _lo1_hopping_enabled);
    _resolve_lox_freq(STAGE_LO2,
        _ch0.lo2_freq_d, _ch1.lo2_freq_d, _ch0.lo2_freq_c, _ch1.lo2_freq_c,
        _ch0.lo2_source, _ch1.lo2_source, _lo2_synth0_mapping, _lo2_synth1_mapping,
        _lo2_hopping_enabled);

    _ctrl->set_lo1_export_source(_lo1_export_src, FORCE_COMMIT);
    _ctrl->set_lo2_export_source(_lo2_export_src, FORCE_COMMIT);
    _ctrl->set_antenna_mapping(_ant_mapping, FORCE_COMMIT);
    //TODO: Re-enable this when we support this mode
    //_ctrl->set_crossover_cal_mode(_cal_mode, FORCE_COMMIT);

    _ctrl->commit();
}

void twinrx_settings_expert::_resolve_lox_freq(
    lo_stage_t                           lo_stage,
    uhd::experts::data_reader_t<double>& ch0_freq_d,
    uhd::experts::data_reader_t<double>& ch1_freq_d,
    uhd::experts::data_writer_t<double>& ch0_freq_c,
    uhd::experts::data_writer_t<double>& ch1_freq_c,
    twinrx_ctrl::lo_source_t             ch0_lo_source,
    twinrx_ctrl::lo_source_t             ch1_lo_source,
    lo_synth_mapping_t                   synth0_mapping,
    lo_synth_mapping_t                   synth1_mapping,
    bool                                 hopping_enabled)
{
    if (ch0_lo_source == twinrx_ctrl::LO_EXTERNAL) {
        // If the LO is external then we don't need to program any synthesizers
        ch0_freq_c = ch0_freq_d;
    } else {
        // When in hopping mode, only attempt to write the LO frequency if it is actually
        // dirty to avoid reconfiguring the LO if it is being "double-buffered". If not
        // hopping, then always write the frequency because other inputs might require
        // an LO re-commit
        const bool freq_update_request = (not hopping_enabled) or ch0_freq_d.is_dirty();
        if (synth0_mapping == MAPPING_CH0 and freq_update_request) {
            ch0_freq_c = _set_lox_synth_freq(lo_stage, twinrx_ctrl::CH1, ch0_freq_d);
        } else if (synth1_mapping == MAPPING_CH0 and freq_update_request) {
            ch0_freq_c = _set_lox_synth_freq(lo_stage, twinrx_ctrl::CH2, ch0_freq_d);
        } else if (synth0_mapping == MAPPING_SHARED or synth1_mapping == MAPPING_SHARED) {
            // If any synthesizer is being shared then we are not in hopping mode
            twinrx_ctrl::channel_t ch = (synth0_mapping == MAPPING_SHARED) ? twinrx_ctrl::CH1 : twinrx_ctrl::CH2;
            ch0_freq_c = _set_lox_synth_freq(lo_stage, ch, ch0_freq_d);
            ch1_freq_c = ch0_freq_c;
        }
    }

    if (ch1_lo_source == twinrx_ctrl::LO_EXTERNAL) {
        // If the LO is external then we don't need to program any synthesizers
        ch1_freq_c = ch1_freq_d;
    } else {
        // When in hopping mode, only attempt to write the LO frequency if it is actually
        // dirty to avoid reconfiguring the LO if it is being "double-buffered". If not
        // hopping, then always write the frequency because other inputs might require
        // an LO re-commit
        const bool freq_update_request = (not hopping_enabled) or ch1_freq_d.is_dirty();
        // As an additional layer of protection from unnecessarily committing the LO, check
        // if the frequency has actually changed.
        if (synth0_mapping == MAPPING_CH1 and freq_update_request) {
            ch1_freq_c = _set_lox_synth_freq(lo_stage, twinrx_ctrl::CH1, ch1_freq_d);
        } else if (synth1_mapping == MAPPING_CH1 and freq_update_request) {
            ch1_freq_c = _set_lox_synth_freq(lo_stage, twinrx_ctrl::CH2, ch1_freq_d);
        } else if (synth0_mapping == MAPPING_SHARED or synth1_mapping == MAPPING_SHARED) {
            // If any synthesizer is being shared then we are not in hopping mode
            twinrx_ctrl::channel_t ch = (synth0_mapping == MAPPING_SHARED) ? twinrx_ctrl::CH1 : twinrx_ctrl::CH2;
            ch0_freq_c = _set_lox_synth_freq(lo_stage, ch, ch0_freq_d);
            ch1_freq_c = ch0_freq_c;
        }
    }
}

double twinrx_settings_expert::_set_lox_synth_freq(lo_stage_t stage, twinrx_ctrl::channel_t ch, double freq)
{
    lo_freq_cache_t* freq_cache = NULL;
    if (stage == STAGE_LO1) {
        freq_cache = (ch == twinrx_ctrl::CH1) ? &_cached_lo1_synth0_freq : &_cached_lo1_synth1_freq;
    } else if (stage == STAGE_LO2) {
        freq_cache = (ch == twinrx_ctrl::CH1) ? &_cached_lo2_synth0_freq : &_cached_lo2_synth1_freq;
    } else {
        throw uhd::assertion_error("Invalid LO stage");
    }

    // Check if the frequency has actually changed before configuring synthesizers
    double coerced_freq = 0.0;
    if (freq_cache->desired != freq) {
        if (stage == STAGE_LO1) {
            coerced_freq = _ctrl->set_lo1_synth_freq(ch, freq, FORCE_COMMIT);
        } else {
            coerced_freq = _ctrl->set_lo2_synth_freq(ch, freq, FORCE_COMMIT);
        }
        freq_cache->desired = rf_freq_ppm_t(freq);
        freq_cache->coerced = coerced_freq;
    } else {
        coerced_freq = freq_cache->coerced;
    }
    return coerced_freq;
}


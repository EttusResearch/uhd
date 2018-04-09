//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_DBOARD_TWINRX_EXPERTS_HPP
#define INCLUDED_DBOARD_TWINRX_EXPERTS_HPP

#include "twinrx_ctrl.hpp"
#include <uhdlib/experts/expert_nodes.hpp>
#include <uhd/utils/math.hpp>

namespace uhd { namespace usrp { namespace dboard { namespace twinrx {

//---------------------------------------------------------
// Misc types and definitions
//---------------------------------------------------------

struct rf_freq_abs_t : public uhd::math::fp_compare::fp_compare_delta<double> {
    rf_freq_abs_t(double freq = 0.0, double epsilon = 1.0 /* 1Hz epsilon */) :
        uhd::math::fp_compare::fp_compare_delta<double>(freq, epsilon) {}
    inline double get() const { return _value; }
};

struct rf_freq_ppm_t : public rf_freq_abs_t {
    rf_freq_ppm_t(double freq = 0.0, double epsilon_ppm = 0.1  /* 1PPM epsilon */) :
        rf_freq_abs_t(freq, 1e-6 * freq * epsilon_ppm) {}
};

enum lo_stage_t { STAGE_LO1, STAGE_LO2 };
enum lo_inj_side_t { INJ_LOW_SIDE, INJ_HIGH_SIDE };
enum lo_synth_mapping_t { MAPPING_NONE, MAPPING_CH0, MAPPING_CH1, MAPPING_SHARED };

static const std::string prepend_ch(std::string name, const std::string& ch) {
    return ch + "/" + name;
}

static const std::string lo_stage_str(lo_stage_t stage, bool lower = false) {
    std::string prefix = lower ? "lo" : "LO";
    return prefix + ((stage == STAGE_LO1) ? "1" : "2");
}


/*!---------------------------------------------------------
 * twinrx_scheduling_expert
 *
 * This expert is responsible for scheduling time sensitive actions
 * in other experts. It responds to changes in the command time and
 * selectively causes experts to run in order to ensure a synchronized
 * system.
 *
 * ---------------------------------------------------------
 */
class twinrx_scheduling_expert : public experts::worker_node_t {
public:
    twinrx_scheduling_expert(const experts::node_retriever_t& db, std::string ch)
    : experts::worker_node_t(prepend_ch("twinrx_scheduling_expert", ch)),
      _command_time     (db, prepend_ch("time/cmd", ch)),
      _rx_frontend_time (db, prepend_ch("time/rx_frontend", ch))
    {
        bind_accessor(_command_time);
        bind_accessor(_rx_frontend_time);
    }

private:
    virtual void resolve();

    //Inputs
    experts::data_reader_t<time_spec_t>    _command_time;

    //Outputs
    experts::data_writer_t<time_spec_t>    _rx_frontend_time;
};

/*!---------------------------------------------------------
 * twinrx_freq_path_expert
 *
 * This expert is responsble for translating a user-specified
 * RF and IF center frequency into TwinRX specific settings
 * like band, preselector path, LO frequency and injection
 * sides for both the LO stages.
 *
 * One instance of this expert is required for each channel
 * ---------------------------------------------------------
 */
class twinrx_freq_path_expert : public experts::worker_node_t {
public:
    twinrx_freq_path_expert(const experts::node_retriever_t& db, std::string ch)
    : experts::worker_node_t(prepend_ch("twinrx_freq_path_expert", ch)),
      _rf_freq_d        (db, prepend_ch("freq/desired", ch)),
      _if_freq_d        (db, prepend_ch("if_freq/desired", ch)),
      _signal_path      (db, prepend_ch("ch/signal_path", ch)),
      _lb_presel        (db, prepend_ch("ch/lb_presel", ch)),
      _hb_presel        (db, prepend_ch("ch/hb_presel", ch)),
      _lb_preamp_presel (db, prepend_ch("ch/lb_preamp_presel", ch)),
      _lo1_freq_d       (db, prepend_ch("los/LO1/freq/desired", ch)),
      _lo2_freq_d       (db, prepend_ch("los/LO2/freq/desired", ch)),
      _lo1_inj_side     (db, prepend_ch("ch/LO1/inj_side", ch)),
      _lo2_inj_side     (db, prepend_ch("ch/LO2/inj_side", ch))
    {
        bind_accessor(_rf_freq_d);
        bind_accessor(_if_freq_d);
        bind_accessor(_signal_path);
        bind_accessor(_lb_presel);
        bind_accessor(_hb_presel);
        bind_accessor(_lb_preamp_presel);
        bind_accessor(_lo1_freq_d);
        bind_accessor(_lo2_freq_d);
        bind_accessor(_lo1_inj_side);
        bind_accessor(_lo2_inj_side);
    }

private:
    virtual void resolve();
    static lo_inj_side_t _compute_lo2_inj_side(
        double lo1_freq, double if1_freq,  double if2_freq, double bandwidth);
    static bool _has_mixer_spurs(
        double lo1_freq, double lo2_freq, double if2_freq,
        double bandwidth, int spur_order);

    //Inputs
    experts::data_reader_t<double>                          _rf_freq_d;
    experts::data_reader_t<double>                          _if_freq_d;
    //Outputs
    experts::data_writer_t<twinrx_ctrl::signal_path_t>      _signal_path;
    experts::data_writer_t<twinrx_ctrl::preselector_path_t> _lb_presel;
    experts::data_writer_t<twinrx_ctrl::preselector_path_t> _hb_presel;
    experts::data_writer_t<bool>                            _lb_preamp_presel;
    experts::data_writer_t<double>                          _lo1_freq_d;
    experts::data_writer_t<double>                          _lo2_freq_d;
    experts::data_writer_t<lo_inj_side_t>                   _lo1_inj_side;
    experts::data_writer_t<lo_inj_side_t>                   _lo2_inj_side;
};

/*!---------------------------------------------------------
 * twinrx_lo_config_expert
 *
 * This expert is responsible for translating high level
 * channel-scoped  LO source and export settings to low-level
 * channel-scoped settings. The expert only deals with
 * the source and export attributes, not frequency.
 *
 * One instance of this expert is required for all channels
 * ---------------------------------------------------------
 */
class twinrx_lo_config_expert : public experts::worker_node_t {
public:
    twinrx_lo_config_expert(const experts::node_retriever_t& db)
    : experts::worker_node_t("twinrx_lo_config_expert"),
      _lo_source_ch0    (db, prepend_ch("los/all/source", "0")),
      _lo_source_ch1    (db, prepend_ch("los/all/source", "1")),
      _lo_export_ch0    (db, prepend_ch("los/all/export", "0")),
      _lo_export_ch1    (db, prepend_ch("los/all/export", "1")),
      _lo1_src_ch0      (db, prepend_ch("ch/LO1/source", "0")),
      _lo1_src_ch1      (db, prepend_ch("ch/LO1/source", "1")),
      _lo2_src_ch0      (db, prepend_ch("ch/LO2/source", "0")),
      _lo2_src_ch1      (db, prepend_ch("ch/LO2/source", "1")),
      _lo1_export_src   (db, "com/LO1/export_source"),
      _lo2_export_src   (db, "com/LO2/export_source")
    {
        bind_accessor(_lo_source_ch0);
        bind_accessor(_lo_source_ch1);
        bind_accessor(_lo_export_ch0);
        bind_accessor(_lo_export_ch1);
        bind_accessor(_lo1_src_ch0);
        bind_accessor(_lo1_src_ch1);
        bind_accessor(_lo2_src_ch0);
        bind_accessor(_lo2_src_ch1);
        bind_accessor(_lo1_export_src);
        bind_accessor(_lo2_export_src);
    }

private:
    virtual void resolve();

    //Inputs
    experts::data_reader_t<std::string>                     _lo_source_ch0;
    experts::data_reader_t<std::string>                     _lo_source_ch1;
    experts::data_reader_t<bool>                            _lo_export_ch0;
    experts::data_reader_t<bool>                            _lo_export_ch1;
    //Outputs
    experts::data_writer_t<twinrx_ctrl::lo_source_t>        _lo1_src_ch0;
    experts::data_writer_t<twinrx_ctrl::lo_source_t>        _lo1_src_ch1;
    experts::data_writer_t<twinrx_ctrl::lo_source_t>        _lo2_src_ch0;
    experts::data_writer_t<twinrx_ctrl::lo_source_t>        _lo2_src_ch1;
    experts::data_writer_t<twinrx_ctrl::lo_export_source_t> _lo1_export_src;
    experts::data_writer_t<twinrx_ctrl::lo_export_source_t> _lo2_export_src;
};

/*!---------------------------------------------------------
 * twinrx_lo_mapping_expert
 *
 * This expert is responsible for translating low-level
 * channel-scoped  LO source and export settings to low-level
 * synthesizer-scoped settings. The expert deals with the
 * extremely flexible channel->synthesizer mapping and handles
 * frequency hopping modes.
 *
 * One instance of this expert is required for each LO stage
 * ---------------------------------------------------------
 */
class twinrx_lo_mapping_expert : public experts::worker_node_t {
public:
    twinrx_lo_mapping_expert(const experts::node_retriever_t& db, lo_stage_t stage)
    : experts::worker_node_t("twinrx_" + lo_stage_str(stage, true) + "_mapping_expert"),
      _lox_src_ch0          (db, prepend_ch("ch/" + lo_stage_str(stage) + "/source", "0")),
      _lox_src_ch1          (db, prepend_ch("ch/" + lo_stage_str(stage) + "/source", "1")),
      _lox_mapping_synth0   (db, prepend_ch("synth/" + lo_stage_str(stage) + "/mapping", "0")),
      _lox_mapping_synth1   (db, prepend_ch("synth/" + lo_stage_str(stage) + "/mapping", "1")),
      _lox_hopping_enabled  (db, "com/synth/" + lo_stage_str(stage) + "/hopping_enabled")
    {
        bind_accessor(_lox_src_ch0);
        bind_accessor(_lox_src_ch1);
        bind_accessor(_lox_mapping_synth0);
        bind_accessor(_lox_mapping_synth1);
        bind_accessor(_lox_hopping_enabled);
    }

private:
    virtual void resolve();

    //Inputs
    experts::data_reader_t<twinrx_ctrl::lo_source_t>    _lox_src_ch0;
    experts::data_reader_t<twinrx_ctrl::lo_source_t>    _lox_src_ch1;
    //Outputs
    experts::data_writer_t<lo_synth_mapping_t>          _lox_mapping_synth0;
    experts::data_writer_t<lo_synth_mapping_t>          _lox_mapping_synth1;
    experts::data_writer_t<bool>                        _lox_hopping_enabled;
};

/*!---------------------------------------------------------
 * twinrx_freq_coercion_expert
 *
 * This expert is responsible for calculating the coerced
 * RF frequency after most settings and modes have been
 * resolved.
 *
 * One instance of this expert is required for each channel
 * ---------------------------------------------------------
 */
class twinrx_freq_coercion_expert : public experts::worker_node_t {
public:
    twinrx_freq_coercion_expert(const experts::node_retriever_t& db, std::string ch)
    : experts::worker_node_t(prepend_ch("twinrx_freq_coercion_expert", ch)),
      _lo1_freq_c       (db, prepend_ch("los/LO1/freq/coerced", ch)),
      _lo2_freq_c       (db, prepend_ch("los/LO2/freq/coerced", ch)),
      _if_freq_d        (db, prepend_ch("if_freq/desired", ch)),
      _lo1_inj_side     (db, prepend_ch("ch/LO1/inj_side", ch)),
      _lo2_inj_side     (db, prepend_ch("ch/LO2/inj_side", ch)),
      _rf_freq_c        (db, prepend_ch("freq/coerced", ch))
    {
        bind_accessor(_lo1_freq_c);
        bind_accessor(_lo2_freq_c);
        bind_accessor(_if_freq_d);
        bind_accessor(_lo1_inj_side);
        bind_accessor(_lo2_inj_side);
        bind_accessor(_rf_freq_c);
    }

private:
    virtual void resolve();

    //Inputs
    experts::data_reader_t<double>          _lo1_freq_c;
    experts::data_reader_t<double>          _lo2_freq_c;
    experts::data_reader_t<double>          _if_freq_d;
    experts::data_reader_t<lo_inj_side_t>   _lo1_inj_side;
    experts::data_reader_t<lo_inj_side_t>   _lo2_inj_side;
    //Outputs
    experts::data_writer_t<double>          _rf_freq_c;
};

/*!---------------------------------------------------------
 * twinrx_nyquist_expert
 *
 * This expert is responsible for figuring out the DSP
 * front-end settings required for each channel
 *
 * One instance of this expert is required for each channel
 * ---------------------------------------------------------
 */
class twinrx_nyquist_expert : public experts::worker_node_t {
public:
    twinrx_nyquist_expert(const experts::node_retriever_t& db, std::string ch,
                          dboard_iface::sptr db_iface)
    : experts::worker_node_t(prepend_ch("twinrx_nyquist_expert", ch)),
      _channel          (ch),
      _codec_conn       (ch=="0"?"II":"QQ"),    //Ch->ADC Port mapping
      _lo1_freq_d       (db, prepend_ch("los/LO1/freq/desired", ch)),
      _lo2_freq_d       (db, prepend_ch("los/LO2/freq/desired", ch)),
      _if_freq_d        (db, prepend_ch("if_freq/desired", ch)),
      _lo1_inj_side     (db, prepend_ch("ch/LO1/inj_side", ch)),
      _lo2_inj_side     (db, prepend_ch("ch/LO2/inj_side", ch)),
      _rx_frontend_time (db, prepend_ch("time/rx_frontend", ch)),
      _if_freq_c        (db, prepend_ch("if_freq/coerced", ch)),
      _db_iface         (db_iface)
    {
        bind_accessor(_lo1_freq_d);
        bind_accessor(_lo2_freq_d);
        bind_accessor(_if_freq_d);
        bind_accessor(_lo1_inj_side);
        bind_accessor(_lo2_inj_side);
        bind_accessor(_if_freq_c);
        bind_accessor(_rx_frontend_time);
    }

private:
    virtual void resolve();

    //Inputs
    const std::string                                       _channel;
    const std::string                                       _codec_conn;
    experts::data_reader_t<double>                          _lo1_freq_d;
    experts::data_reader_t<double>                          _lo2_freq_d;
    experts::data_reader_t<double>                          _if_freq_d;
    experts::data_reader_t<lo_inj_side_t>                   _lo1_inj_side;
    experts::data_reader_t<lo_inj_side_t>                   _lo2_inj_side;
    experts::data_reader_t<time_spec_t>                     _rx_frontend_time;

    //Outputs
    experts::data_writer_t<double>                          _if_freq_c;
    dboard_iface::sptr                                      _db_iface;

    //Misc
    time_spec_t  _cached_cmd_time;
};

/*!---------------------------------------------------------
 * twinrx_antenna_expert
 *
 * This expert is responsible for translating high-level
 * antenna selection settings and channel enables to low-level
 * switch configurations.
 *
 * One instance of this expert is required for all channels
 * ---------------------------------------------------------
 */
class twinrx_antenna_expert : public experts::worker_node_t {
public:
    twinrx_antenna_expert(const experts::node_retriever_t& db)
    : experts::worker_node_t("twinrx_antenna_expert"),
      _antenna_ch0      (db, prepend_ch("antenna", "0")),
      _antenna_ch1      (db, prepend_ch("antenna", "1")),
      _enabled_ch0      (db, prepend_ch("enabled", "0")),
      _enabled_ch1      (db, prepend_ch("enabled", "1")),
      _lo_export_ch0    (db, prepend_ch("los/all/export", "0")),
      _lo_export_ch1    (db, prepend_ch("los/all/export", "1")),
      _ant_mapping      (db, "com/ant_mapping"),
      _cal_mode         (db, "com/cal_mode")
    {
        bind_accessor(_antenna_ch0);
        bind_accessor(_antenna_ch1);
        bind_accessor(_enabled_ch0);
        bind_accessor(_enabled_ch1);
        bind_accessor(_lo_export_ch0);
        bind_accessor(_lo_export_ch1);
        bind_accessor(_ant_mapping);
        bind_accessor(_cal_mode);
    }

private:
    virtual void resolve();

    //Inputs
    experts::data_reader_t<std::string>                     _antenna_ch0;
    experts::data_reader_t<std::string>                     _antenna_ch1;
    experts::data_reader_t<bool>                            _enabled_ch0;
    experts::data_reader_t<bool>                            _enabled_ch1;
    experts::data_reader_t<bool>                            _lo_export_ch0;
    experts::data_reader_t<bool>                            _lo_export_ch1;
    //Outputs
    experts::data_writer_t<twinrx_ctrl::antenna_mapping_t>  _ant_mapping;
    experts::data_writer_t<twinrx_ctrl::cal_mode_t>         _cal_mode;
};

/*!---------------------------------------------------------
 * twinrx_chan_gain_expert
 *
 * This expert is responsible for mapping high-level channel
 * gain settings to individual attenuator and amp configurations
 * that are also channel-scoped. This expert will implement
 * the gain distribution strategy.
 *
 * One instance of this expert is required for each channel
 * ---------------------------------------------------------
 */
class twinrx_chan_gain_expert : public experts::worker_node_t {
public:
    twinrx_chan_gain_expert(const experts::node_retriever_t& db, std::string ch)
    : experts::worker_node_t(prepend_ch("twinrx_chan_gain_expert", ch)),
      _gain         (db, prepend_ch("gain", ch)),
      _gain_profile (db, prepend_ch("gain_profile", ch)),
      _signal_path  (db, prepend_ch("ch/signal_path", ch)),
      _lb_presel    (db, prepend_ch("ch/lb_presel", ch)),
      _hb_presel    (db, prepend_ch("ch/hb_presel", ch)),
      _ant_mapping  (db, "com/ant_mapping"),
      _input_atten  (db, prepend_ch("ch/input_atten", ch)),
      _lb_atten     (db, prepend_ch("ch/lb_atten", ch)),
      _hb_atten     (db, prepend_ch("ch/hb_atten", ch)),
      _preamp1      (db, prepend_ch("ch/preamp1", ch)),
      _preamp2      (db, prepend_ch("ch/preamp2", ch))
    {
        bind_accessor(_gain);
        bind_accessor(_gain_profile);
        bind_accessor(_signal_path);
        bind_accessor(_lb_presel);
        bind_accessor(_hb_presel);
        bind_accessor(_ant_mapping);
        bind_accessor(_input_atten);
        bind_accessor(_lb_atten);
        bind_accessor(_hb_atten);
        bind_accessor(_preamp1);
        bind_accessor(_preamp2);
    }

private:
    virtual void resolve();

    //Inputs
    experts::data_reader_t<double>                          _gain;
    experts::data_reader_t<std::string>                     _gain_profile;
    experts::data_reader_t<twinrx_ctrl::signal_path_t>      _signal_path;
    experts::data_reader_t<twinrx_ctrl::preselector_path_t> _lb_presel;
    experts::data_reader_t<twinrx_ctrl::preselector_path_t> _hb_presel;
    experts::data_reader_t<twinrx_ctrl::antenna_mapping_t>  _ant_mapping;
    //Outputs
    experts::data_writer_t<uint8_t>                  _input_atten;
    experts::data_writer_t<uint8_t>                  _lb_atten;
    experts::data_writer_t<uint8_t>                  _hb_atten;
    experts::data_writer_t<twinrx_ctrl::preamp_state_t>     _preamp1;
    experts::data_writer_t<bool>                            _preamp2;
};

/*!---------------------------------------------------------
 * twinrx_ant_gain_expert
 *
 * This expert is responsible for translating between the
 * channel-scoped low-level gain settings to antenna-scoped
 * gain settings.
 *
 * One instance of this expert is required for all channels
 * ---------------------------------------------------------
 */
class twinrx_ant_gain_expert : public experts::worker_node_t {
public:
    twinrx_ant_gain_expert(const experts::node_retriever_t& db)
    : experts::worker_node_t("twinrx_ant_gain_expert"),
      _ant_mapping          (db, "com/ant_mapping"),
      _ch0_input_atten      (db, prepend_ch("ch/input_atten", "0")),
      _ch0_preamp1          (db, prepend_ch("ch/preamp1", "0")),
      _ch0_preamp2          (db, prepend_ch("ch/preamp2", "0")),
      _ch0_lb_preamp_presel (db, prepend_ch("ch/lb_preamp_presel", "0")),
      _ch1_input_atten      (db, prepend_ch("ch/input_atten", "1")),
      _ch1_preamp1          (db, prepend_ch("ch/preamp1", "1")),
      _ch1_preamp2          (db, prepend_ch("ch/preamp2", "1")),
      _ch1_lb_preamp_presel (db, prepend_ch("ch/lb_preamp_presel", "1")),
      _ant0_input_atten     (db, prepend_ch("ant/input_atten", "0")),
      _ant0_preamp1         (db, prepend_ch("ant/preamp1", "0")),
      _ant0_preamp2         (db, prepend_ch("ant/preamp2", "0")),
      _ant0_lb_preamp_presel(db, prepend_ch("ant/lb_preamp_presel", "0")),
      _ant1_input_atten     (db, prepend_ch("ant/input_atten", "1")),
      _ant1_preamp1         (db, prepend_ch("ant/preamp1", "1")),
      _ant1_preamp2         (db, prepend_ch("ant/preamp2", "1")),
      _ant1_lb_preamp_presel(db, prepend_ch("ant/lb_preamp_presel", "1"))
    {
        bind_accessor(_ant_mapping);
        bind_accessor(_ch0_input_atten);
        bind_accessor(_ch0_preamp1);
        bind_accessor(_ch0_preamp2);
        bind_accessor(_ch0_lb_preamp_presel);
        bind_accessor(_ch1_input_atten);
        bind_accessor(_ch1_preamp1);
        bind_accessor(_ch1_preamp2);
        bind_accessor(_ch1_lb_preamp_presel);
        bind_accessor(_ant0_input_atten);
        bind_accessor(_ant0_preamp1);
        bind_accessor(_ant0_preamp2);
        bind_accessor(_ant0_lb_preamp_presel);
        bind_accessor(_ant1_input_atten);
        bind_accessor(_ant1_preamp1);
        bind_accessor(_ant1_preamp2);
        bind_accessor(_ant1_lb_preamp_presel);
    }

private:
    virtual void resolve();

    //Inputs
    experts::data_reader_t<twinrx_ctrl::antenna_mapping_t>  _ant_mapping;
    experts::data_reader_t<uint8_t>                  _ch0_input_atten;
    experts::data_reader_t<twinrx_ctrl::preamp_state_t>     _ch0_preamp1;
    experts::data_reader_t<bool>                            _ch0_preamp2;
    experts::data_reader_t<bool>                            _ch0_lb_preamp_presel;
    experts::data_reader_t<uint8_t>                  _ch1_input_atten;
    experts::data_reader_t<twinrx_ctrl::preamp_state_t>     _ch1_preamp1;
    experts::data_reader_t<bool>                            _ch1_preamp2;
    experts::data_reader_t<bool>                            _ch1_lb_preamp_presel;

    //Outputs
    experts::data_writer_t<uint8_t>                  _ant0_input_atten;
    experts::data_writer_t<twinrx_ctrl::preamp_state_t>     _ant0_preamp1;
    experts::data_writer_t<bool>                            _ant0_preamp2;
    experts::data_writer_t<bool>                            _ant0_lb_preamp_presel;
    experts::data_writer_t<uint8_t>                  _ant1_input_atten;
    experts::data_writer_t<twinrx_ctrl::preamp_state_t>     _ant1_preamp1;
    experts::data_writer_t<bool>                            _ant1_preamp2;
    experts::data_writer_t<bool>                            _ant1_lb_preamp_presel;
};

/*!---------------------------------------------------------
 * twinrx_settings_expert
 *
 * This expert is responsible for gathering all low-level
 * settings and writing them to hardware. All LO frequency
 * settings are cached with a hysteresis. All other settings
 * are always written to twinrx_ctrl and rely on register
 * level caching.
 *
 * One instance of this expert is required for all channels
 * ---------------------------------------------------------
 */
class twinrx_settings_expert : public experts::worker_node_t {
public:
    twinrx_settings_expert(const experts::node_retriever_t& db, twinrx_ctrl::sptr ctrl)
    : experts::worker_node_t("twinrx_settings_expert"), _ctrl(ctrl),
      _ch0              (db, "0"),
      _ch1              (db, "1"),
      _lo1_synth0_mapping(db, "0/synth/LO1/mapping"),
      _lo1_synth1_mapping(db, "1/synth/LO1/mapping"),
      _lo2_synth0_mapping(db, "0/synth/LO2/mapping"),
      _lo2_synth1_mapping(db, "1/synth/LO2/mapping"),
      _lo1_hopping_enabled(db, "com/synth/LO1/hopping_enabled"),
      _lo2_hopping_enabled(db, "com/synth/LO2/hopping_enabled"),
      _lo1_export_src   (db, "com/LO1/export_source"),
      _lo2_export_src   (db, "com/LO2/export_source"),
      _ant_mapping      (db, "com/ant_mapping"),
      _cal_mode         (db, "com/cal_mode")
    {
        for (size_t i = 0; i < 2; i++) {
            ch_settings& ch = (i==1) ? _ch1 : _ch0;
            bind_accessor(ch.chan_enabled);
            bind_accessor(ch.preamp1);
            bind_accessor(ch.preamp2);
            bind_accessor(ch.lb_preamp_presel);
            bind_accessor(ch.signal_path);
            bind_accessor(ch.lb_presel);
            bind_accessor(ch.hb_presel);
            bind_accessor(ch.input_atten);
            bind_accessor(ch.lb_atten);
            bind_accessor(ch.hb_atten);
            bind_accessor(ch.lo1_source);
            bind_accessor(ch.lo2_source);
            bind_accessor(ch.lo1_freq_d);
            bind_accessor(ch.lo2_freq_d);
            bind_accessor(ch.lo1_freq_c);
            bind_accessor(ch.lo2_freq_c);
        }
        bind_accessor(_lo1_synth0_mapping);
        bind_accessor(_lo1_synth1_mapping);
        bind_accessor(_lo2_synth0_mapping);
        bind_accessor(_lo2_synth1_mapping);
        bind_accessor(_lo1_hopping_enabled);
        bind_accessor(_lo2_hopping_enabled);
        bind_accessor(_lo1_export_src);
        bind_accessor(_lo2_export_src);
        bind_accessor(_ant_mapping);
        bind_accessor(_cal_mode);
    }

private:
    virtual void resolve();
    void _resolve_lox_freq(
        lo_stage_t                      lo_stage,
        experts::data_reader_t<double>& ch0_freq_d,
        experts::data_reader_t<double>& ch1_freq_d,
        experts::data_writer_t<double>& ch0_freq_c,
        experts::data_writer_t<double>& ch1_freq_c,
        twinrx_ctrl::lo_source_t        ch0_lo_source,
        twinrx_ctrl::lo_source_t        ch1_lo_source,
        lo_synth_mapping_t              synth0_mapping,
        lo_synth_mapping_t              synth1_mapping,
        bool                            hopping_enabled);
    double _set_lox_synth_freq(lo_stage_t stage, twinrx_ctrl::channel_t ch, double freq);

    class ch_settings {
    public:
        ch_settings(const experts::node_retriever_t& db, const std::string& ch) :
            chan_enabled    (db, prepend_ch("enabled", ch)),
            preamp1         (db, prepend_ch("ant/preamp1", ch)),
            preamp2         (db, prepend_ch("ant/preamp2", ch)),
            lb_preamp_presel(db, prepend_ch("ant/lb_preamp_presel", ch)),
            signal_path     (db, prepend_ch("ch/signal_path", ch)),
            lb_presel       (db, prepend_ch("ch/lb_presel", ch)),
            hb_presel       (db, prepend_ch("ch/hb_presel", ch)),
            input_atten     (db, prepend_ch("ant/input_atten", ch)),
            lb_atten        (db, prepend_ch("ch/lb_atten", ch)),
            hb_atten        (db, prepend_ch("ch/hb_atten", ch)),
            lo1_source      (db, prepend_ch("ch/LO1/source", ch)),
            lo2_source      (db, prepend_ch("ch/LO2/source", ch)),
            lo1_freq_d      (db, prepend_ch("los/LO1/freq/desired", ch)),
            lo2_freq_d      (db, prepend_ch("los/LO2/freq/desired", ch)),
            lo1_freq_c      (db, prepend_ch("los/LO1/freq/coerced", ch)),
            lo2_freq_c      (db, prepend_ch("los/LO2/freq/coerced", ch))
        {}

        //Inputs (channel specific)
        experts::data_reader_t<bool>                            chan_enabled;
        experts::data_reader_t<twinrx_ctrl::preamp_state_t>     preamp1;
        experts::data_reader_t<bool>                            preamp2;
        experts::data_reader_t<bool>                            lb_preamp_presel;
        experts::data_reader_t<twinrx_ctrl::signal_path_t>      signal_path;
        experts::data_reader_t<twinrx_ctrl::preselector_path_t> lb_presel;
        experts::data_reader_t<twinrx_ctrl::preselector_path_t> hb_presel;
        experts::data_reader_t<uint8_t>                  input_atten;
        experts::data_reader_t<uint8_t>                  lb_atten;
        experts::data_reader_t<uint8_t>                  hb_atten;
        experts::data_reader_t<twinrx_ctrl::lo_source_t>        lo1_source;
        experts::data_reader_t<twinrx_ctrl::lo_source_t>        lo2_source;
        experts::data_reader_t<double>                          lo1_freq_d;
        experts::data_reader_t<double>                          lo2_freq_d;

        //Output (channel specific)
        experts::data_writer_t<double>                          lo1_freq_c;
        experts::data_writer_t<double>                          lo2_freq_c;
    };

    //External interface
    twinrx_ctrl::sptr                                       _ctrl;

    //Inputs (channel agnostic)
    ch_settings                                             _ch0;
    ch_settings                                             _ch1;
    experts::data_reader_t<lo_synth_mapping_t>              _lo1_synth0_mapping;
    experts::data_reader_t<lo_synth_mapping_t>              _lo1_synth1_mapping;
    experts::data_reader_t<lo_synth_mapping_t>              _lo2_synth0_mapping;
    experts::data_reader_t<lo_synth_mapping_t>              _lo2_synth1_mapping;
    experts::data_reader_t<bool>                            _lo1_hopping_enabled;
    experts::data_reader_t<bool>                            _lo2_hopping_enabled;
    experts::data_reader_t<twinrx_ctrl::lo_export_source_t> _lo1_export_src;
    experts::data_reader_t<twinrx_ctrl::lo_export_source_t> _lo2_export_src;
    experts::data_reader_t<twinrx_ctrl::antenna_mapping_t>  _ant_mapping;
    experts::data_reader_t<twinrx_ctrl::cal_mode_t>         _cal_mode;

    //Outputs (channel agnostic)
    //None

    //Misc
    struct lo_freq_cache_t {
        rf_freq_ppm_t desired;
        double        coerced;
    };
    lo_freq_cache_t  _cached_lo1_synth0_freq;
    lo_freq_cache_t  _cached_lo2_synth0_freq;
    lo_freq_cache_t  _cached_lo1_synth1_freq;
    lo_freq_cache_t  _cached_lo2_synth1_freq;

    static const bool FORCE_COMMIT;
};


}}}} //namespaces

#endif /* INCLUDED_DBOARD_TWINRX_EXPERTS_HPP */

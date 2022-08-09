//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "zbx_constants.hpp"
#include "zbx_cpld_ctrl.hpp"
#include "zbx_lo_ctrl.hpp"
#include <uhd/cal/container.hpp>
#include <uhd/cal/dsa_cal.hpp>
#include <uhd/experts/expert_nodes.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/usrp/zbx_tune_map_item.hpp>
#include <uhdlib/rfnoc/rf_control/gain_profile_iface.hpp>
#include <uhdlib/usrp/common/pwr_cal_mgr.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/usrp/common/x400_rfdc_control.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <cmath>
#include <memory>

namespace uhd { namespace usrp { namespace zbx {

namespace {

//! Depending on the given \p lo_step_size, this will return a valid frequency
// range on a quantized grid for the the LOs. The lower limit of this range will
// never be smaller than LMX2572_MIN_FREQ and the upper frequency will never be
// larger than LMX2572_MAX_FREQ. All frequencies will be integer multiples of
// the given \p lo_step_size.
uhd::freq_range_t _get_quantized_lo_range(const double lo_step_size)
{
    const double start = std::ceil(LMX2572_MIN_FREQ / lo_step_size) * lo_step_size;
    const double stop  = std::floor(LMX2572_MAX_FREQ / lo_step_size) * lo_step_size;
    UHD_ASSERT_THROW(start >= LMX2572_MIN_FREQ);
    UHD_ASSERT_THROW(stop <= LMX2572_MAX_FREQ);
    return uhd::freq_range_t(start, stop, lo_step_size);
}

} // namespace

/*!---------------------------------------------------------
 * zbx_scheduling_expert
 *
 * This expert is responsible for scheduling time sensitive actions
 * in other experts. It responds to changes in the command time and
 * selectively causes experts to run in order to ensure a synchronized
 * system.
 *
 * There is one scheduling expert per channel, they are shared between RX and TX.
 * So, 2 scheduling experts total per radio block.
 * ---------------------------------------------------------
 */
class zbx_scheduling_expert : public experts::worker_node_t
{
public:
    zbx_scheduling_expert(const experts::node_retriever_t& db, const uhd::fs_path fe_path)
        : experts::worker_node_t(fe_path / "zbx_scheduling_expert")
        , _command_time(db, fe_path / "time/cmd")
        , _frontend_time(db, fe_path / "time/fe")
    {
        bind_accessor(_command_time);
        bind_accessor(_frontend_time);
    }

private:
    virtual void resolve();

    // Inputs
    experts::data_reader_t<time_spec_t> _command_time;

    // Outputs
    experts::data_writer_t<time_spec_t> _frontend_time;
};

/*!---------------------------------------------------------
 * zbx_freq_fe_expert (Frequency Front-end Expert)
 *
 * This expert is responsible for responding to user requests for center frequency tuning
 *
 * This should trigger:
 *    - relevant LO experts
 *    - adjacent MPM expert
 *    - adjacent CPLD (tx/rx) Programming expert
 * After all of the above, the Frequency Backend expert should be triggered to returned
 * the coerced center frequency
 *
 * One instance of this expert is required for each combination of Direction (TX/RX) and
 * Channel (0,1); four total
 * --------------------------------------------------------
 */
class zbx_freq_fe_expert : public uhd::experts::worker_node_t
{
public:
    zbx_freq_fe_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const uhd::direction_t trx,
        const size_t chan,
        const double rfdc_rate,
        const double lo_step_size)
        : experts::worker_node_t(fe_path / "zbx_freq_fe_expert")
        , _desired_frequency(db, fe_path / "freq" / "desired")
        , _tune_table(db, fe_path / "tune_table")
        , _desired_lo1_frequency(
              db, fe_path / "los" / ZBX_LO1 / "freq" / "value" / "desired")
        , _desired_lo2_frequency(
              db, fe_path / "los" / ZBX_LO2 / "freq" / "value" / "desired")
        , _lo1_enabled(db, fe_path / ZBX_LO1 / "enabled")
        , _lo2_enabled(db, fe_path / ZBX_LO2 / "enabled")
        , _desired_if2_frequency(db, fe_path / "if_freq" / "desired")
        , _band_inverted(db, fe_path / "band_inverted")
        , _is_highband(db, fe_path / "is_highband")
        , _lo1_inj_side(db, fe_path / "lo1_inj_side")
        , _lo2_inj_side(db, fe_path / "lo2_inj_side")
        , _rf_filter(db, fe_path / "rf" / "filter")
        , _if1_filter(db, fe_path / "if1" / "filter")
        , _if2_filter(db, fe_path / "if2" / "filter")
        , _rfdc_rate(rfdc_rate)
        , _lo_freq_range(_get_quantized_lo_range(lo_step_size))
        , _trx(trx)
        , _chan(chan)
    {
        //  Inputs
        bind_accessor(_desired_frequency);
        bind_accessor(_tune_table);

        //  Outputs
        bind_accessor(_desired_lo1_frequency);
        bind_accessor(_desired_lo2_frequency);
        bind_accessor(_lo1_enabled);
        bind_accessor(_lo2_enabled);
        bind_accessor(_desired_if2_frequency);
        bind_accessor(_band_inverted);
        bind_accessor(_is_highband);
        bind_accessor(_lo1_inj_side);
        bind_accessor(_lo2_inj_side);
        bind_accessor(_rf_filter);
        bind_accessor(_if1_filter);
        bind_accessor(_if2_filter);
    }

private:
    void resolve() override;

    // Inputs from user/API
    uhd::experts::data_reader_t<double> _desired_frequency;
    uhd::experts::data_reader_t<std::vector<zbx_tune_map_item_t>> _tune_table;

    // Outputs
    // From calculation, to LO expert
    uhd::experts::data_writer_t<double> _desired_lo1_frequency;
    uhd::experts::data_writer_t<double> _desired_lo2_frequency;
    uhd::experts::data_writer_t<bool> _lo1_enabled;
    uhd::experts::data_writer_t<bool> _lo2_enabled;
    // From calculation, to MPM/RPC expert
    uhd::experts::data_writer_t<double> _desired_if2_frequency;
    uhd::experts::data_writer_t<bool> _band_inverted;
    // From calculation, to Frequency Backend expert
    uhd::experts::data_writer_t<bool> _is_highband;
    uhd::experts::data_writer_t<lo_inj_side_t> _lo1_inj_side;
    uhd::experts::data_writer_t<lo_inj_side_t> _lo2_inj_side;
    // From calculation, to CPLD Programming expert
    uhd::experts::data_writer_t<int> _rf_filter;
    uhd::experts::data_writer_t<int> _if1_filter;
    uhd::experts::data_writer_t<int> _if2_filter;

    const double _rfdc_rate;
    const uhd::freq_range_t _lo_freq_range;
    zbx_tune_map_item_t _tune_settings;
    // Channel properties
    const uhd::direction_t _trx;
    const size_t _chan;
};

/*!---------------------------------------------------------
 * zbx_freq_be_expert (Frequency Back-end Expert)
 *
 * This expert is responsible for calculating the final coerced frequency and returning it
 * to the user
 *
 * This should trigger:
 *    - adjacent gain expert
 *
 * One instance of this expert is required for each combination of Direction (TX/RX) and
 * Channel (0,1); four total
 * --------------------------------------------------------
 */
class zbx_freq_be_expert : public uhd::experts::worker_node_t
{
public:
    zbx_freq_be_expert(
        const uhd::experts::node_retriever_t& db, const uhd::fs_path fe_path)
        : uhd::experts::worker_node_t(fe_path / "zbx_freq_be_expert")
        , _coerced_lo1_frequency(
              db, fe_path / "los" / ZBX_LO1 / "freq" / "value" / "coerced")
        , _coerced_lo2_frequency(
              db, fe_path / "los" / ZBX_LO2 / "freq" / "value" / "coerced")
        , _coerced_if2_frequency(db, fe_path / "if_freq" / "coerced")
        , _is_highband(db, fe_path / "is_highband")
        , _lo1_inj_side(db, fe_path / "lo1_inj_side")
        , _lo2_inj_side(db, fe_path / "lo2_inj_side")
        , _coerced_frequency(db, fe_path / "freq" / "coerced")
    {
        //  Inputs
        bind_accessor(_coerced_lo1_frequency);
        bind_accessor(_coerced_lo2_frequency);
        bind_accessor(_coerced_if2_frequency);
        bind_accessor(_is_highband);
        bind_accessor(_lo1_inj_side);
        bind_accessor(_lo2_inj_side);

        //  Outputs
        bind_accessor(_coerced_frequency);
    }

private:
    void resolve() override;

    // Inputs from LO expert(s)
    uhd::experts::data_reader_t<double> _coerced_lo1_frequency;
    uhd::experts::data_reader_t<double> _coerced_lo2_frequency;
    // Input from MPM/RPC expert
    uhd::experts::data_reader_t<double> _coerced_if2_frequency;
    uhd::experts::data_reader_t<bool> _is_highband;
    // Input from Frequency FE
    uhd::experts::data_reader_t<lo_inj_side_t> _lo1_inj_side;
    uhd::experts::data_reader_t<lo_inj_side_t> _lo2_inj_side;

    // Output to user/API
    uhd::experts::data_writer_t<double> _coerced_frequency;
};

/*!---------------------------------------------------------
 * zbx_lo_expert
 *
 * This expert is responsible for controlling one LO on the zbx
 * note: LO source control is handled by the CPLD Programming Expert
 *
 * This should trigger:
 *    - Relevant (tx/rx, channel) Frequency Back-end Expert
 *
 * One instance of this expert is required for each LO (lo1, lo2) per Direction (TX/RX)
 * and Channel (0,1); eight total
 * --------------------------------------------------------
 */
class zbx_lo_expert : public uhd::experts::worker_node_t
{
public:
    zbx_lo_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const std::string lo,
        std::shared_ptr<zbx_lo_ctrl> zbx_lo_ctrl)
        : uhd::experts::worker_node_t(fe_path / "zbx_" + lo + "_expert")
        , _desired_lo_frequency(db, fe_path / "los" / lo / "freq" / "value" / "desired")
        , _set_is_enabled(db, fe_path / lo / "enabled")
        , _test_mode_enabled(db, fe_path / lo / "test_mode")
        , _coerced_lo_frequency(db, fe_path / "los" / lo / "freq" / "value" / "coerced")
        , _lo_ctrl(zbx_lo_ctrl)
    {
        bind_accessor(_desired_lo_frequency);
        bind_accessor(_test_mode_enabled);
        bind_accessor(_set_is_enabled);
        bind_accessor(_coerced_lo_frequency);
    }

private:
    void resolve() override;

    // Inputs from Frequency FE expert or user/API
    uhd::experts::data_reader_t<double> _desired_lo_frequency;
    uhd::experts::data_reader_t<bool> _set_is_enabled;
    // Inputs from user/API
    uhd::experts::data_reader_t<bool> _test_mode_enabled;

    // Outputs to Frequency BE expert or user/API
    uhd::experts::data_writer_t<double> _coerced_lo_frequency;

    std::shared_ptr<zbx_lo_ctrl> _lo_ctrl;
};


/*! DSA coercer expert
 *
 * Knows how to coerce a DSA value.
 */
class zbx_gain_coercer_expert : public uhd::experts::worker_node_t
{
public:
    zbx_gain_coercer_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path gain_path,
        const uhd::meta_range_t valid_range)
        : uhd::experts::worker_node_t(gain_path / "zbx_gain_coercer_expert")
        , _gain_desired(db, gain_path / "desired")
        , _gain_coerced(db, gain_path / "coerced")
        , _valid_range(valid_range)
    {
        bind_accessor(_gain_desired);
        bind_accessor(_gain_coerced);
    }

private:
    void resolve() override;
    // Input
    uhd::experts::data_reader_t<double> _gain_desired;
    // Output
    uhd::experts::data_writer_t<double> _gain_coerced;
    // Attributes
    const uhd::meta_range_t _valid_range;
};

/*!---------------------------------------------------------
 * zbx_tx_gain_expert (TX Gain Expert)
 *
 * This expert is responsible for controlling the gain of each TX channel.
 * If the gain profile is set to default, then it will look up the corresponding
 * amp and DSA values and write them to those nodes.
 *
 * This should trigger:
 *    - Adjacent CPLD TX Programming Expert
 *
 * One instance of this expert is required for each TX Channel (0,1); two total
 * --------------------------------------------------------
 */
class zbx_tx_gain_expert : public uhd::experts::worker_node_t
{
public:
    zbx_tx_gain_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const size_t chan,
        uhd::usrp::pwr_cal_mgr::sptr power_mgr,
        uhd::usrp::cal::zbx_tx_dsa_cal::sptr dsa_cal)
        : uhd::experts::worker_node_t(fe_path / "zbx_gain_expert")
        , _gain_in(db, fe_path / "gains" / ZBX_GAIN_STAGE_ALL / "value" / "desired")
        , _profile(db, fe_path / "gains" / "all" / "profile")
        , _frequency(db, fe_path / "freq" / "coerced")
        , _gain_out(db, fe_path / "gains" / ZBX_GAIN_STAGE_ALL / "value" / "coerced")
        , _dsa1(db, fe_path / "gains" / ZBX_GAIN_STAGE_DSA1 / "value" / "desired")
        , _dsa2(db, fe_path / "gains" / ZBX_GAIN_STAGE_DSA2 / "value" / "desired")
        , _amp_gain(db, fe_path / "gains" / ZBX_GAIN_STAGE_AMP / "value" / "desired")
        , _power_mgr(power_mgr)
        , _dsa_cal(dsa_cal)
        , _chan(chan)
    {
        bind_accessor(_gain_in);
        bind_accessor(_profile);
        bind_accessor(_frequency);
        bind_accessor(_gain_out);
        bind_accessor(_dsa1);
        bind_accessor(_dsa2);
        bind_accessor(_amp_gain);
    }

private:
    void resolve() override;
    void _set_tx_dsa(const std::string, const uint8_t desired_gain);
    double _set_tx_amp_by_gain(const double gain);
    // Inputs from user/API
    uhd::experts::data_reader_t<double> _gain_in;
    // Inputs for DSA calibration
    uhd::experts::data_reader_t<std::string> _profile;
    uhd::experts::data_reader_t<double> _frequency;

    // Output to user/API
    uhd::experts::data_writer_t<double> _gain_out;
    // Outputs to CPLD programming expert
    uhd::experts::data_writer_t<double> _dsa1;
    uhd::experts::data_writer_t<double> _dsa2;
    uhd::experts::data_writer_t<double> _amp_gain;

    uhd::usrp::pwr_cal_mgr::sptr _power_mgr;
    uhd::usrp::cal::zbx_tx_dsa_cal::sptr _dsa_cal;
    const size_t _chan;
};

/*!---------------------------------------------------------
 * zbx_rx_gain_expert (RX Gain Expert)
 *
 * This expert is responsible for controlling the gain of each RX channel
 *
 * This should trigger:
 *    - Adjacent CPLD RX Programming Expert
 *
 * One instance of this expert is required for each RX Channel (0,1); two total
 * --------------------------------------------------------
 */
class zbx_rx_gain_expert : public uhd::experts::worker_node_t
{
public:
    zbx_rx_gain_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        uhd::usrp::pwr_cal_mgr::sptr power_mgr,
        uhd::usrp::cal::zbx_rx_dsa_cal::sptr dsa_cal)
        : uhd::experts::worker_node_t(fe_path / "zbx_gain_expert")
        , _gain_in(db, fe_path / "gains" / ZBX_GAIN_STAGE_ALL / "value" / "desired")
        , _profile(db, fe_path / "gains" / "all" / "profile")
        , _frequency(db, fe_path / "freq" / "coerced")
        , _gain_out(db, fe_path / "gains" / ZBX_GAIN_STAGE_ALL / "value" / "coerced")
        , _dsa1(db, fe_path / "gains" / ZBX_GAIN_STAGE_DSA1 / "value" / "desired")
        , _dsa2(db, fe_path / "gains" / ZBX_GAIN_STAGE_DSA2 / "value" / "desired")
        , _dsa3a(db, fe_path / "gains" / ZBX_GAIN_STAGE_DSA3A / "value" / "desired")
        , _dsa3b(db, fe_path / "gains" / ZBX_GAIN_STAGE_DSA3B / "value" / "desired")
        , _power_mgr(power_mgr)
        , _dsa_cal(dsa_cal)
    {
        bind_accessor(_gain_in);
        bind_accessor(_profile);
        bind_accessor(_frequency);
        bind_accessor(_gain_out);
        bind_accessor(_dsa1);
        bind_accessor(_dsa2);
        bind_accessor(_dsa3a);
        bind_accessor(_dsa3b);
    }

private:
    void resolve() override;

    // Inputs from user/API
    uhd::experts::data_reader_t<double> _gain_in;
    uhd::experts::data_reader_t<std::string> _profile;
    // Inputs for dsa calibration
    uhd::experts::data_reader_t<double> _frequency;

    // Output to user/API
    uhd::experts::data_writer_t<double> _gain_out;
    // Outputs to CPLD programming expert
    uhd::experts::data_writer_t<double> _dsa1;
    uhd::experts::data_writer_t<double> _dsa2;
    uhd::experts::data_writer_t<double> _dsa3a;
    uhd::experts::data_writer_t<double> _dsa3b;

    uhd::usrp::pwr_cal_mgr::sptr _power_mgr;
    uhd::usrp::cal::zbx_rx_dsa_cal::sptr _dsa_cal;
};

/*!---------------------------------------------------------
 * zbx_tx_programming_expert (TX CPLD Programming Expert)
 *
 * This expert is responsible for programming the ZBX CPLD with parameters determined by
 * user input or other experts This includes antenna setting, gain/dsa steps, lo source
 * control, rf filter settings
 *
 * This expert should not trigger any other experts, these are all blind parameters
 *
 * One instance of this expert is required for each TX Channel (0,1); two total
 * --------------------------------------------------------
 */
class zbx_tx_programming_expert : public uhd::experts::worker_node_t
{
public:
    zbx_tx_programming_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path tx_fe_path,
        const uhd::fs_path rx_fe_path, /*needed for shared command time*/
        const size_t chan,
        uhd::usrp::cal::zbx_tx_dsa_cal::sptr dsa_cal,
        std::shared_ptr<zbx_cpld_ctrl> cpld)
        : experts::worker_node_t(tx_fe_path / "zbx_tx_programming_expert")
        , _antenna(db, tx_fe_path / "antenna" / "value")
        , _atr_mode(db, tx_fe_path / "atr_mode")
        , _profile(db, tx_fe_path / "gains" / "all" / "profile")
        , _command_time(db, rx_fe_path / "time" / "cmd")
        , _frequency(db, tx_fe_path / "freq" / "coerced")
        , _dsa1(db, tx_fe_path / "gains" / ZBX_GAIN_STAGE_DSA1 / "value" / "coerced")
        , _dsa2(db, tx_fe_path / "gains" / ZBX_GAIN_STAGE_DSA2 / "value" / "coerced")
        , _amp_gain(db, tx_fe_path / "gains" / ZBX_GAIN_STAGE_AMP / "value" / "coerced")
        , _rf_filter(db, tx_fe_path / "rf" / "filter")
        , _if1_filter(db, tx_fe_path / "if1" / "filter")
        , _if2_filter(db, tx_fe_path / "if2" / "filter")
        , _is_highband(db, tx_fe_path / "is_highband")
        , _lo1_source(db, tx_fe_path / "ch" / ZBX_LO1 / "source")
        , _lo2_source(db, tx_fe_path / "ch" / ZBX_LO2 / "source")
        , _dsa_cal(dsa_cal)
        , _cpld(cpld)
        , _chan(chan)
    {
        bind_accessor(_antenna);
        bind_accessor(_atr_mode);
        bind_accessor(_profile);
        bind_accessor(_command_time);
        bind_accessor(_frequency);
        bind_accessor(_dsa1);
        bind_accessor(_dsa2);
        bind_accessor(_amp_gain);
        bind_accessor(_rf_filter);
        bind_accessor(_if1_filter);
        bind_accessor(_if2_filter);
        bind_accessor(_is_highband);
        bind_accessor(_lo1_source);
        bind_accessor(_lo2_source);
    }

private:
    void resolve() override;

    // Inputs from user/API
    uhd::experts::data_reader_t<std::string> _antenna;
    uhd::experts::data_reader_t<zbx_cpld_ctrl::atr_mode> _atr_mode;
    uhd::experts::data_reader_t<std::string> _profile;

    // Inputs from the Frequency FE expert
    // Note: this is just for node dependencies, we want to be notified if just the tune
    // frequency has been changed.
    uhd::experts::data_reader_t<time_spec_t> _command_time;
    uhd::experts::data_reader_t<double> _frequency;

    // Inputs from Gain TX expert
    uhd::experts::data_reader_t<double> _dsa1;
    uhd::experts::data_reader_t<double> _dsa2;
    uhd::experts::data_reader_t<double> _amp_gain;

    // Inputs from Frequency FE expert
    uhd::experts::data_reader_t<int> _rf_filter;
    uhd::experts::data_reader_t<int> _if1_filter;
    uhd::experts::data_reader_t<int> _if2_filter;
    uhd::experts::data_reader_t<bool> _is_highband;
    // Inputs from LO expert(s)
    uhd::experts::data_reader_t<zbx_lo_source_t> _lo1_source;
    uhd::experts::data_reader_t<zbx_lo_source_t> _lo2_source;

    uhd::usrp::cal::zbx_tx_dsa_cal::sptr _dsa_cal;
    // Expects constructed cpld control objects
    std::shared_ptr<zbx_cpld_ctrl> _cpld;
    const size_t _chan;
};

/*!---------------------------------------------------------
 * zbx_rx_programming_expert (RX CPLD Programming Expert)
 *
 * This expert is responsible for programming the ZBX CPLD with parameters determined by
 * user input or other experts.
 * This includes antenna setting, gain/dsa steps, lo source control, rf filter settings
 *
 * This expert should not trigger any other experts, these are all blind parameters
 *
 * One instance of this expert is required for each RX Channel (0,1); two total
 * --------------------------------------------------------
 */
class zbx_rx_programming_expert : public uhd::experts::worker_node_t
{
public:
    zbx_rx_programming_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const size_t chan,
        uhd::usrp::cal::zbx_rx_dsa_cal::sptr dsa_cal,
        std::shared_ptr<zbx_cpld_ctrl> cpld)
        : experts::worker_node_t(fe_path / "zbx_rx_programming_expert")
        , _antenna(db, fe_path / "antenna" / "value")
        , _atr_mode(db, fe_path / "atr_mode")
        , _profile(db, fe_path / "gains" / "all" / "profile")
        , _command_time(db, fe_path / "time" / "cmd")
        , _frequency(db, fe_path / "freq" / "coerced")
        , _dsa1(db, fe_path / "gains" / ZBX_GAIN_STAGE_DSA1 / "value" / "coerced")
        , _dsa2(db, fe_path / "gains" / ZBX_GAIN_STAGE_DSA2 / "value" / "coerced")
        , _dsa3a(db, fe_path / "gains" / ZBX_GAIN_STAGE_DSA3A / "value" / "coerced")
        , _dsa3b(db, fe_path / "gains" / ZBX_GAIN_STAGE_DSA3B / "value" / "coerced")
        , _rf_filter(db, fe_path / "rf" / "filter")
        , _if1_filter(db, fe_path / "if1" / "filter")
        , _if2_filter(db, fe_path / "if2" / "filter")
        , _is_highband(db, fe_path / "is_highband")
        , _lo1_source(db, fe_path / "ch" / ZBX_LO1 / "source")
        , _lo2_source(db, fe_path / "ch" / ZBX_LO2 / "source")
        , _dsa_cal(dsa_cal)
        , _cpld(cpld)
        , _chan(chan)
    {
        bind_accessor(_antenna);
        bind_accessor(_atr_mode);
        bind_accessor(_profile);
        bind_accessor(_command_time);
        bind_accessor(_frequency);
        bind_accessor(_dsa1);
        bind_accessor(_dsa2);
        bind_accessor(_dsa3a);
        bind_accessor(_dsa3b);
        bind_accessor(_rf_filter);
        bind_accessor(_if1_filter);
        bind_accessor(_if2_filter);
        bind_accessor(_is_highband);
        bind_accessor(_lo1_source);
        bind_accessor(_lo2_source);
    }

private:
    void resolve() override;
    void _update_leds();

    // Inputs from user/API
    uhd::experts::data_reader_t<std::string> _antenna;
    uhd::experts::data_reader_t<zbx_cpld_ctrl::atr_mode> _atr_mode;
    uhd::experts::data_reader_t<std::string> _profile;

    // Inputs from the Frequency FE expert
    // Note: this is just for node dependencies, we want to be notified if just the tune
    // frequency has been changed.
    uhd::experts::data_reader_t<time_spec_t> _command_time;
    uhd::experts::data_reader_t<double> _frequency;

    // Inputs from Gain expert
    uhd::experts::data_reader_t<double> _dsa1;
    uhd::experts::data_reader_t<double> _dsa2;
    uhd::experts::data_reader_t<double> _dsa3a;
    uhd::experts::data_reader_t<double> _dsa3b;

    // Inputs from Frequency FE expert
    uhd::experts::data_reader_t<int> _rf_filter;
    uhd::experts::data_reader_t<int> _if1_filter;
    uhd::experts::data_reader_t<int> _if2_filter;
    uhd::experts::data_reader_t<bool> _is_highband;
    // Inputs from LO expert(s)
    uhd::experts::data_reader_t<zbx_lo_source_t> _lo1_source;
    uhd::experts::data_reader_t<zbx_lo_source_t> _lo2_source;

    uhd::usrp::cal::zbx_rx_dsa_cal::sptr _dsa_cal;
    // Expects constructed cpld control objects
    std::shared_ptr<zbx_cpld_ctrl> _cpld;
    const size_t _chan;
};

/*!---------------------------------------------------------
 * zbx_band_inversion_expert
 *
 * This expert is responsible for handling the band inversion calls to MPM on the target
 * device
 *
 * This expert should not trigger any others
 *
 * One instance of this expert is required for each Direction (TX/RX) and Channel (0,1);
 * four total
 * --------------------------------------------------------
 */
class zbx_band_inversion_expert : public uhd::experts::worker_node_t
{
public:
    zbx_band_inversion_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const uhd::direction_t trx,
        const size_t chan,
        uhd::usrp::zbx_rpc_iface::sptr rpcc)
        : uhd::experts::worker_node_t(fe_path / "zbx_band_inversion_expert")
        , _is_band_inverted(db, fe_path / "band_inverted")
        , _rpcc(rpcc)
        , _trx(trx)
        , _chan(chan)
    {
        bind_accessor(_is_band_inverted);
    }

private:
    void resolve() override;

    // Inputs from Frequency FE expert
    uhd::experts::data_reader_t<bool> _is_band_inverted;

    uhd::usrp::zbx_rpc_iface::sptr _rpcc;
    const uhd::direction_t _trx;
    const size_t _chan;
};

/*!---------------------------------------------------------
 * zbx_rfdc_freq_expert
 *
 * This expert is responsible for handling any rfdc frequency calls to MPM on the target
 * device
 *
 * This expert should not trigger any experts
 *
 * One instance of this expert is required for each Direction (TX/RX) and Channel (0,1);
 * four total
 * --------------------------------------------------------
 */
class zbx_rfdc_freq_expert : public uhd::experts::worker_node_t
{
public:
    zbx_rfdc_freq_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const uhd::direction_t trx,
        const size_t chan,
        const std::string rpc_prefix,
        int db_idx,
        uhd::usrp::x400_rpc_iface::sptr rpcc)
        : uhd::experts::worker_node_t(fe_path / "zbx_rfdc_freq_expert")
        , _rfdc_freq_desired(
              db, fe_path / "los" / RFDC_NCO / "freq" / "value" / "desired")
        , _rfdc_freq_coerced(
              db, fe_path / "los" / RFDC_NCO / "freq" / "value" / "coerced")
        , _if2_frequency_desired(db, fe_path / "if_freq" / "desired")
        , _if2_frequency_coerced(db, fe_path / "if_freq" / "coerced")
        , _rpc_prefix(rpc_prefix)
        , _db_idx(db_idx)
        , _rpcc(rpcc)
        , _trx(trx)
        , _chan(chan)
    {
        bind_accessor(_rfdc_freq_desired);
        bind_accessor(_rfdc_freq_coerced);
        bind_accessor(_if2_frequency_desired);
        bind_accessor(_if2_frequency_coerced);
    }

private:
    void resolve() override;

    // Inputs from user/API
    uhd::experts::data_reader_t<double> _rfdc_freq_desired;

    // Outputs to user/API
    uhd::experts::data_writer_t<double> _rfdc_freq_coerced;


    // Inputs from Frequency FE expert
    uhd::experts::data_reader_t<double> _if2_frequency_desired;

    // Outputs to Frequency BE expert
    uhd::experts::data_writer_t<double> _if2_frequency_coerced;


    const std::string _rpc_prefix;
    const size_t _db_idx;
    uhd::usrp::x400_rpc_iface::sptr _rpcc;
    const uhd::direction_t _trx;
    const size_t _chan;
};

using uhd::rfnoc::x400::rfdc_control;
/*!---------------------------------------------------------
 * zbx_sync_expert
 *
 * This expert is responsible for handling the phase alignment.
 * Per channel, there are up to 4 things whose phase need syncing: The two
 * LOs, the NCO, and the ADC/DAC gearboxes. However, the LOs share a sync
 * register, and so do the NCOs.  To minimize writes, we thus need a single sync
 * expert at the end of the graph, who combines all LOs and all NCOs.
 * --------------------------------------------------------
 */
class zbx_sync_expert : public uhd::experts::worker_node_t
{
public:
    zbx_sync_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path tx_fe_path,
        const uhd::fs_path rx_fe_path,
        rfdc_control::sptr rfdcc,
        std::shared_ptr<zbx_cpld_ctrl> cpld)
        : uhd::experts::worker_node_t("zbx_sync_expert")
        , _fe_time{{db, rx_fe_path / 0 / "time/fe"}, {db, rx_fe_path / 1 / "time/fe"}}
        , _lo_freqs{{zbx_lo_t::RX0_LO1,
                        {db,
                            rx_fe_path / 0 / "los" / ZBX_LO1 / "freq" / "value"
                                / "coerced"}},
              {zbx_lo_t::RX0_LO2,
                  {db, rx_fe_path / 0 / "los" / ZBX_LO2 / "freq" / "value" / "coerced"}},
              {zbx_lo_t::TX0_LO1,
                  {db, tx_fe_path / 0 / "los" / ZBX_LO1 / "freq" / "value" / "coerced"}},
              {zbx_lo_t::TX0_LO2,
                  {db, tx_fe_path / 0 / "los" / ZBX_LO2 / "freq" / "value" / "coerced"}},
              {zbx_lo_t::RX1_LO1,
                  {db, rx_fe_path / 1 / "los" / ZBX_LO1 / "freq" / "value" / "coerced"}},
              {zbx_lo_t::RX1_LO2,
                  {db, rx_fe_path / 1 / "los" / ZBX_LO2 / "freq" / "value" / "coerced"}},
              {zbx_lo_t::TX1_LO1,
                  {db, tx_fe_path / 1 / "los" / ZBX_LO1 / "freq" / "value" / "coerced"}},
              {zbx_lo_t::TX1_LO2,
                  {db, tx_fe_path / 1 / "los" / ZBX_LO2 / "freq" / "value" / "coerced"}}}
        , _nco_freqs{{rfdc_control::rfdc_type::RX0,
                         {db, rx_fe_path / 0 / "if_freq" / "coerced"}},
              {rfdc_control::rfdc_type::RX1,
                  {db, rx_fe_path / 1 / "if_freq" / "coerced"}},
              {rfdc_control::rfdc_type::TX0,
                  {db, tx_fe_path / 0 / "if_freq" / "coerced"}},
              {rfdc_control::rfdc_type::TX1,
                  {db, tx_fe_path / 1 / "if_freq" / "coerced"}}}
        , _rfdcc(rfdcc)
        , _cpld(cpld)
    {
        for (auto& fe_time : _fe_time) {
            bind_accessor(fe_time);
        }
        for (auto& lo_freq : _lo_freqs) {
            bind_accessor(lo_freq.second);
        }
        for (auto& nco_freq : _nco_freqs) {
            bind_accessor(nco_freq.second);
        }
    }

private:
    void resolve() override;

    // Inputs from user/API
    // Command time: We have 2 channels, one time spec per channel
    std::vector<uhd::experts::data_reader_t<time_spec_t>> _fe_time;
    // We have 8 LOs:
    std::map<zbx_lo_t, uhd::experts::data_reader_t<double>> _lo_freqs;
    // We have 4 NCOs
    std::map<rfdc_control::rfdc_type, uhd::experts::data_reader_t<double>> _nco_freqs;

    // This expert has no outputs.

    // Attributes
    rfdc_control::sptr _rfdcc;
    std::shared_ptr<zbx_cpld_ctrl> _cpld;
    //! Store the sync state of the ADC gearboxes. If false, we assume they're
    // out of sync. This could also be a vector of booleans if we want to be
    // able to sync ADC gearboxes individually.
    bool _adcs_synced = false;
    //! Store the sync state of the DAC gearboxes. If false, we assume they're
    // out of sync. This could also be a vector of booleans if we want to be
    // able to sync DAC gearboxes individually.
    bool _dacs_synced = false;
};

}}} // namespace uhd::usrp::zbx

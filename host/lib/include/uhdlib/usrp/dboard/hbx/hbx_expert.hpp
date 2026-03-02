//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "hbx_admv1320_ctrl.hpp"
#include "hbx_admv1420_ctrl.hpp"
#include "hbx_constants.hpp"
#include "hbx_cpld_ctrl.hpp"
#include "hbx_demod_ctrl.hpp"
#include "hbx_lo_ctrl.hpp"
#include <uhd/experts/expert_nodes.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/types/iq_dc_cal_coeffs.hpp>
#include <uhdlib/usrp/common/pwr_cal_mgr.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/usrp/common/x400_rfdc_control.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_cpld_ctrl.hpp>
#include <cmath>
#include <complex>
#include <memory>

namespace uhd { namespace usrp { namespace hbx {

/*!---------------------------------------------------------
 * hbx_scheduling_expert
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
class hbx_scheduling_expert : public experts::worker_node_t
{
public:
    hbx_scheduling_expert(const experts::node_retriever_t& db, const uhd::fs_path fe_path)
        : experts::worker_node_t(fe_path / "hbx_scheduling_expert")
        , _command_time(db, fe_path / "time/cmd")
        , _frontend_time(db, fe_path / "time/fe")
    {
        bind_accessor(_command_time);
        bind_accessor(_frontend_time);
    }

private:
    virtual void resolve() override;

    // Inputs:
    experts::data_reader_t<time_spec_t> _command_time;

    // Outputs:
    experts::data_writer_t<time_spec_t> _frontend_time;
};

/*-----------------------------------------------------------------------------
 * hbx_lo_expert
 *
 * This expert is responsible for controlling one LO on the hbx
 *
 * This should trigger:
 *   - Relevant (rx/rx) Frquency Back-end Expert
 *
 * One instance of this expert is required for each LO per Direction (TX/RX), two in
 * total.
 */
class hbx_lo_expert : public experts::worker_node_t
{
public:
    hbx_lo_expert(const experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const uhd::direction_t trx,
        const std::string lo,
        std::shared_ptr<hbx_lo_ctrl> hbx_lo_ctrl,
        std::shared_ptr<hbx_cpld_ctrl> cpld)
        : experts::worker_node_t(fe_path / "hbx_" + lo + "_expert")
        , _desired_lo_frequency(db, fe_path / "los" / lo / "freq" / "value" / "desired")
        , _lo_export(db, fe_path / "los" / lo / "export")
        , _lo_import(db, fe_path / "los" / lo / "import")
        , _desired_lo_ext_power(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_EXT / "value" / "desired")
        , _coerced_lo_frequency(db, fe_path / "los" / lo / "freq" / "value" / "coerced")
        , _trx(trx)
        , _lo_ctrl(hbx_lo_ctrl)
        , _cpld(cpld)
    {
        bind_accessor(_desired_lo_frequency);
        bind_accessor(_lo_export);
        bind_accessor(_lo_import);
        bind_accessor(_desired_lo_ext_power);
        bind_accessor(_coerced_lo_frequency);
    }


private:
    void resolve() override;

    // Inputs from Frequency FE expert or user/API
    uhd::experts::data_reader_t<double> _desired_lo_frequency;
    uhd::experts::data_reader_t<bool> _lo_export;
    uhd::experts::data_reader_t<bool> _lo_import;

    // Outputs to Frequency BE expert or user/API
    uhd::experts::data_writer_t<double> _desired_lo_ext_power;
    uhd::experts::data_writer_t<double> _coerced_lo_frequency;

    uhd::direction_t _trx;
    std::shared_ptr<hbx_lo_ctrl> _lo_ctrl;
    std::shared_ptr<hbx_cpld_ctrl> _cpld;
};

/*!----------------------------------------------------------------------------
 * hbx_tx_gain_programming_expert
 *
 * This expert is responsible for handling any gain programming calls.
 *
 * This expert should not trigger any experts.
 */
class hbx_tx_gain_programming_expert : public uhd::experts::worker_node_t
{
public:
    hbx_tx_gain_programming_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        std::shared_ptr<hbx_cpld_ctrl> hbx_cpld_ctrl,
        std::shared_ptr<hbx_admv1320_ctrl> admv1320_ctrl,
        std::shared_ptr<hbx_lo_ctrl> hbx_lo_ctrl)
        : uhd::experts::worker_node_t(fe_path / "hbx_tx_gain_programming_expert")
        , _rf_dsa_in(db, fe_path / "gains" / HBX_GAIN_STAGE_RF / "value" / "desired")
        , _lo_dsa_in(db, fe_path / "gains" / HBX_GAIN_STAGE_LO / "value" / "desired")
        , _lo_pwr_int_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_INT / "value" / "desired")
        , _lo_pwr_ext_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_EXT / "value" / "desired")
        , _admv_dsa_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA_ALL / "value" / "desired")
        , _admv_dsa1_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA1 / "value" / "desired")
        , _admv_dsa2_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA2 / "value" / "desired")
        , _rf_dsa_out(db, fe_path / "gains" / HBX_GAIN_STAGE_RF / "value" / "coerced")
        , _lo_dsa_out(db, fe_path / "gains" / HBX_GAIN_STAGE_LO / "value" / "coerced")
        , _lo_pwr_int_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_INT / "value" / "coerced")
        , _lo_pwr_ext_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_EXT / "value" / "coerced")
        , _admv_dsa_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA_ALL / "value" / "coerced")
        , _admv_dsa1_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA1 / "value" / "coerced")
        , _admv_dsa2_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA2 / "value" / "coerced")
        , _cpld(hbx_cpld_ctrl)
        , _admv1320(admv1320_ctrl)
        , _lo_ctrl(hbx_lo_ctrl)
    {
        bind_accessor(_rf_dsa_in);
        bind_accessor(_lo_dsa_in);
        bind_accessor(_lo_pwr_int_in);
        bind_accessor(_lo_pwr_ext_in);
        bind_accessor(_admv_dsa_in);
        bind_accessor(_admv_dsa1_in);
        bind_accessor(_admv_dsa2_in);

        bind_accessor(_rf_dsa_out);
        bind_accessor(_lo_dsa_out);
        bind_accessor(_lo_pwr_int_out);
        bind_accessor(_lo_pwr_ext_out);
        bind_accessor(_admv_dsa_out);
        bind_accessor(_admv_dsa1_out);
        bind_accessor(_admv_dsa2_out);
    }

private:
    void resolve() override;
    // Inputs from user/API
    uhd::experts::data_reader_t<double> _rf_dsa_in;
    uhd::experts::data_reader_t<double> _lo_dsa_in;
    uhd::experts::data_reader_t<double> _lo_pwr_int_in;
    uhd::experts::data_reader_t<double> _lo_pwr_ext_in;
    uhd::experts::data_reader_t<double> _admv_dsa_in;
    uhd::experts::data_reader_t<double> _admv_dsa1_in;
    uhd::experts::data_reader_t<double> _admv_dsa2_in;

    // Outputs
    uhd::experts::data_writer_t<double> _rf_dsa_out;
    uhd::experts::data_writer_t<double> _lo_dsa_out;
    uhd::experts::data_writer_t<double> _lo_pwr_int_out;
    uhd::experts::data_writer_t<double> _lo_pwr_ext_out;
    uhd::experts::data_writer_t<double> _admv_dsa_out;
    uhd::experts::data_writer_t<double> _admv_dsa1_out;
    uhd::experts::data_writer_t<double> _admv_dsa2_out;
    // Expects constructed control objects
    std::shared_ptr<hbx_cpld_ctrl> _cpld;
    std::shared_ptr<hbx_admv1320_ctrl> _admv1320;
    std::shared_ptr<hbx_lo_ctrl> _lo_ctrl;
};

/*!----------------------------------------------------------------------------
 * hbx_rx_gain_programming_expert
 *
 * This expert is responsible for handling any gain programming calls.
 *
 * This expert should not trigger any experts.
 */
class hbx_rx_gain_programming_expert : public uhd::experts::worker_node_t
{
public:
    hbx_rx_gain_programming_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        std::shared_ptr<hbx_cpld_ctrl> hbx_cpld_ctrl,
        std::shared_ptr<hbx_admv1420_ctrl> admv1420_ctrl,
        std::shared_ptr<hbx_lo_ctrl> hbx_lo_ctrl)
        : uhd::experts::worker_node_t(fe_path / "hbx_rx_gain_programming_expert")
        , _rf_dsa_in(db, fe_path / "gains" / HBX_GAIN_STAGE_RF / "value" / "desired")
        , _lo_dsa_in(db, fe_path / "gains" / HBX_GAIN_STAGE_LO / "value" / "desired")
        , _lo_pwr_int_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_INT / "value" / "desired")
        , _lo_pwr_ext_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_EXT / "value" / "desired")
        , _lf_dsa1(db, fe_path / "gains" / HBX_GAIN_STAGE_LF_DSA1 / "value" / "desired")
        , _lf_dsa2(db, fe_path / "gains" / HBX_GAIN_STAGE_LF_DSA2 / "value" / "desired")
        , _admv_dsa_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA_ALL / "value" / "desired")
        , _admv_dsa2_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA2 / "value" / "desired")
        , _admv_dsa3_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA3 / "value" / "desired")
        , _admv_dsa4_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA4 / "value" / "desired")
        , _admv_dsa5_in(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA5 / "value" / "desired")
        , _rf_dsa_out(db, fe_path / "gains" / HBX_GAIN_STAGE_RF / "value" / "coerced")
        , _lo_dsa_out(db, fe_path / "gains" / HBX_GAIN_STAGE_LO / "value" / "coerced")
        , _lo_pwr_int_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_INT / "value" / "coerced")
        , _lo_pwr_ext_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_EXT / "value" / "coerced")
        , _lf_dsa1_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LF_DSA1 / "value" / "coerced")
        , _lf_dsa2_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LF_DSA2 / "value" / "coerced")
        , _admv_dsa_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA_ALL / "value" / "coerced")
        , _admv_dsa2_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA2 / "value" / "coerced")
        , _admv_dsa3_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA3 / "value" / "coerced")
        , _admv_dsa4_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA4 / "value" / "coerced")
        , _admv_dsa5_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA5 / "value" / "coerced")
        , _cpld(hbx_cpld_ctrl)
        , _admv1420(admv1420_ctrl)
        , _lo_ctrl(hbx_lo_ctrl)
    {
        bind_accessor(_rf_dsa_in);
        bind_accessor(_lo_dsa_in);
        bind_accessor(_lo_pwr_int_in);
        bind_accessor(_lo_pwr_ext_in);
        bind_accessor(_lf_dsa1);
        bind_accessor(_lf_dsa2);
        bind_accessor(_admv_dsa_in);
        bind_accessor(_admv_dsa2_in);
        bind_accessor(_admv_dsa3_in);
        bind_accessor(_admv_dsa4_in);
        bind_accessor(_admv_dsa5_in);

        bind_accessor(_rf_dsa_out);
        bind_accessor(_lo_dsa_out);
        bind_accessor(_lo_pwr_int_out);
        bind_accessor(_lo_pwr_ext_out);
        bind_accessor(_lf_dsa1_out);
        bind_accessor(_lf_dsa2_out);
        bind_accessor(_admv_dsa_out);
        bind_accessor(_admv_dsa2_out);
        bind_accessor(_admv_dsa3_out);
        bind_accessor(_admv_dsa4_out);
        bind_accessor(_admv_dsa5_out);
    }

private:
    void resolve() override;

    // Inputs
    uhd::experts::data_reader_t<double> _rf_dsa_in;
    uhd::experts::data_reader_t<double> _lo_dsa_in;
    uhd::experts::data_reader_t<double> _lo_pwr_int_in;
    uhd::experts::data_reader_t<double> _lo_pwr_ext_in;
    uhd::experts::data_reader_t<double> _lf_dsa1;
    uhd::experts::data_reader_t<double> _lf_dsa2;
    uhd::experts::data_reader_t<double> _admv_dsa_in;
    uhd::experts::data_reader_t<double> _admv_dsa2_in;
    uhd::experts::data_reader_t<double> _admv_dsa3_in;
    uhd::experts::data_reader_t<double> _admv_dsa4_in;
    uhd::experts::data_reader_t<double> _admv_dsa5_in;

    // Outputs
    uhd::experts::data_writer_t<double> _rf_dsa_out;
    uhd::experts::data_writer_t<double> _lo_dsa_out;
    uhd::experts::data_writer_t<double> _lo_pwr_int_out;
    uhd::experts::data_writer_t<double> _lo_pwr_ext_out;
    uhd::experts::data_writer_t<double> _lf_dsa1_out;
    uhd::experts::data_writer_t<double> _lf_dsa2_out;
    uhd::experts::data_writer_t<double> _admv_dsa_out;
    uhd::experts::data_writer_t<double> _admv_dsa2_out;
    uhd::experts::data_writer_t<double> _admv_dsa3_out;
    uhd::experts::data_writer_t<double> _admv_dsa4_out;
    uhd::experts::data_writer_t<double> _admv_dsa5_out;

    // Expects constructed control objects
    std::shared_ptr<hbx_cpld_ctrl> _cpld;
    std::shared_ptr<hbx_admv1420_ctrl> _admv1420;
    std::shared_ptr<hbx_lo_ctrl> _lo_ctrl;
};

/*!----------------------------------------------------------------------------
 * hbx_tx_gain_expert
 *
 * This expert is responsible for handling the overall gain settings for TX.
 * This expert is going to trigger the hbx_tx_gain_programming_expert which will then set
 * all individual gain stages according to what is set here.
 *
 */
class hbx_tx_gain_expert : public uhd::experts::worker_node_t
{
public:
    hbx_tx_gain_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        uhd::usrp::pwr_cal_mgr::sptr power_mgr)
        : uhd::experts::worker_node_t(fe_path / "hbx_tx_gain_expert")
        , _gain_all_in(db, fe_path / "gains" / HBX_GAIN_STAGE_ALL / "value" / "desired")
        , _profile(db, fe_path / "gains" / "all" / "profile")
        , _freq(db, fe_path / "freq" / "coerced")
        , _lo_import(db, fe_path / "los" / HBX_LO / "import")
        , _gain_all_out(db, fe_path / "gains" / HBX_GAIN_STAGE_ALL / "value" / "coerced")
        , _rf_dsa_gain_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_RF / "value" / "desired")
        , _admv_gain_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA_ALL / "value" / "desired")
        , _desired_lo_int_power(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_INT / "value" / "desired")
        , _desired_lo_gain(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO / "value" / "desired")
        , _power_mgr(power_mgr)
    {
        bind_accessor(_gain_all_in);
        bind_accessor(_profile);
        bind_accessor(_freq);
        bind_accessor(_lo_import);

        bind_accessor(_gain_all_out);
        bind_accessor(_rf_dsa_gain_out);
        bind_accessor(_admv_gain_out);
        bind_accessor(_desired_lo_int_power);
        bind_accessor(_desired_lo_gain);
    }

private:
    void resolve() override;
    // Inputs from user/API
    uhd::experts::data_reader_t<double> _gain_all_in;
    uhd::experts::data_reader_t<std::string> _profile;
    uhd::experts::data_reader_t<double> _freq;
    uhd::experts::data_reader_t<bool> _lo_import;

    // Outputs to user/API
    uhd::experts::data_writer_t<double> _gain_all_out;
    // Outputs to other experts
    uhd::experts::data_writer_t<double> _rf_dsa_gain_out;
    uhd::experts::data_writer_t<double> _admv_gain_out;
    uhd::experts::data_writer_t<double> _desired_lo_int_power;
    uhd::experts::data_writer_t<double> _desired_lo_gain;

    uhd::usrp::pwr_cal_mgr::sptr _power_mgr;
};

/*!----------------------------------------------------------------------------
 * hbx_rx_gain_expert
 *
 * This expert is responsible for handling the overall gain settings for RX.
 * This expert is going to trigger the hbx_rx_gain_programming_expert which will then set
 * all individual gain stages according to what is set here.
 *
 */
class hbx_rx_gain_expert : public uhd::experts::worker_node_t
{
public:
    hbx_rx_gain_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        uhd::usrp::pwr_cal_mgr::sptr power_mgr)
        : uhd::experts::worker_node_t(fe_path / "hbx_rx_gain_expert")
        , _gain_all_in(db, fe_path / "gains" / HBX_GAIN_STAGE_ALL / "value" / "desired")
        , _profile(db, fe_path / "gains" / "all" / "profile")
        , _freq(db, fe_path / "freq" / "coerced")
        , _lo_import(db, fe_path / "los" / HBX_LO / "import")
        , _gain_all_out(db, fe_path / "gains" / HBX_GAIN_STAGE_ALL / "value" / "coerced")
        , _lf_dsa1_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LF_DSA1 / "value" / "desired")
        , _lf_dsa2_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LF_DSA2 / "value" / "desired")
        , _rf_dsa_out(db, fe_path / "gains" / HBX_GAIN_STAGE_RF / "value" / "desired")
        , _admv_dsa_out(
              db, fe_path / "gains" / HBX_GAIN_STAGE_ADMV_DSA_ALL / "value" / "desired")
        , _desired_lo_int_power(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO_PWR_INT / "value" / "desired")
        , _desired_lo_gain(
              db, fe_path / "gains" / HBX_GAIN_STAGE_LO / "value" / "desired")
        , _power_mgr(power_mgr)
    {
        bind_accessor(_gain_all_in);
        bind_accessor(_profile);
        bind_accessor(_freq);
        bind_accessor(_lo_import);

        bind_accessor(_gain_all_out);
        bind_accessor(_lf_dsa1_out);
        bind_accessor(_lf_dsa2_out);
        bind_accessor(_rf_dsa_out);
        bind_accessor(_admv_dsa_out);
        bind_accessor(_desired_lo_int_power);
        bind_accessor(_desired_lo_gain);
    }

private:
    void resolve() override;
    // Inputs from user/API
    uhd::experts::data_reader_t<double> _gain_all_in;
    uhd::experts::data_reader_t<std::string> _profile;
    uhd::experts::data_reader_t<double> _freq;
    uhd::experts::data_reader_t<bool> _lo_import;

    // Outputs to user/API
    uhd::experts::data_writer_t<double> _gain_all_out;
    // Outputs to other experts
    uhd::experts::data_writer_t<double> _lf_dsa1_out;
    uhd::experts::data_writer_t<double> _lf_dsa2_out;
    uhd::experts::data_writer_t<double> _rf_dsa_out;
    uhd::experts::data_writer_t<double> _admv_dsa_out;
    uhd::experts::data_writer_t<double> _desired_lo_int_power;
    uhd::experts::data_writer_t<double> _desired_lo_gain;

    uhd::usrp::pwr_cal_mgr::sptr _power_mgr;
};

/*!----------------------------------------------------------------------------
 * hbx_rfdc_freq_expert
 *
 * This expert is responsible for handling any rfdc frequency calls to MPM on the
 * target device.
 *
 * This expert should not trigger any experts.
 *
 * One instance of this expert is required for each Direction (TX/RX).
 */
class hbx_rfdc_freq_expert : public uhd::experts::worker_node_t
{
public:
    hbx_rfdc_freq_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const uhd::direction_t trx,
        int db_idx,
        uhd::usrp::x400_rpc_iface::sptr rpcc)
        : uhd::experts::worker_node_t(fe_path / "hbx_rfdc_freq_expert")
        , _rfdc_freq_desired(
              db, fe_path / "los" / RFDC_NCO / "freq" / "value" / "desired")
        , _rfdc_freq_coerced(
              db, fe_path / "los" / RFDC_NCO / "freq" / "value" / "coerced")
        , _db_idx(db_idx)
        , _rpcc(rpcc)
        , _trx(trx)
    {
        bind_accessor(_rfdc_freq_desired);
        bind_accessor(_rfdc_freq_coerced);
    }

private:
    void resolve() override;

    // Inputs from user/API
    uhd::experts::data_reader_t<double> _rfdc_freq_desired;

    // Outputs to user/API
    uhd::experts::data_writer_t<double> _rfdc_freq_coerced;

    const size_t _db_idx;
    uhd::usrp::x400_rpc_iface::sptr _rpcc;
    const uhd::direction_t _trx;
};

/*!----------------------------------------------------------------------------
 * hbx_rx_programming_expert (RX Programming Expert)
 *
 * This expert is responsible for programming the HBX with parameters determined by
 * user input or other experts. This includes antenna and LED settings.
 *
 * This expert should not trigger any other experts, these are all blind parameters.
 *
 * One instance of this expert is required for one HBX daughterboard.
 */
class hbx_rx_programming_expert : public uhd::experts::worker_node_t
{
public:
    hbx_rx_programming_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        std::shared_ptr<hbx_cpld_ctrl> hbx_cpld_ctrl)
        : experts::worker_node_t(fe_path / "hbx_rx_programming_expert")
        , _antenna(db, fe_path / "antenna" / "value")
        , _command_time(db, fe_path / "time" / "cmd")
        , _cpld(hbx_cpld_ctrl)
    {
        bind_accessor(_antenna);
        bind_accessor(_command_time);
    }

private:
    void resolve() override;
    void _update_leds();

    // Inputs from user/API
    uhd::experts::data_reader_t<std::string> _antenna;
    uhd::experts::data_reader_t<time_spec_t> _command_time;

    // Expects constructed control objects
    std::shared_ptr<hbx_cpld_ctrl> _cpld;
};

/*!----------------------------------------------------------------------------
 * hbx_tx_programming_expert (TX Programming Expert)
 *
 * This expert is responsible for programming the HBX with parameters determined by
 * user input or other experts. This includes antenna and LED setting.
 *
 * This expert should not trigger any other experts, these are all blind parameters.
 *
 * One instance of this expert is required for one HBX daughterboard.
 */
class hbx_tx_programming_expert : public uhd::experts::worker_node_t
{
public:
    hbx_tx_programming_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        std::shared_ptr<hbx_cpld_ctrl> hbx_cpld_ctrl)
        : experts::worker_node_t(fe_path / "hbx_tx_programming_expert")
        , _antenna(db, fe_path / "antenna" / "value")
        , _cpld(hbx_cpld_ctrl)
    {
        bind_accessor(_antenna);
    }

private:
    void resolve() override;

    // Inputs from user/API
    uhd::experts::data_reader_t<std::string> _antenna;

    // Expects constructed control objects
    std::shared_ptr<hbx_cpld_ctrl> _cpld;
};


class hbx_tx_band_expert : public uhd::experts::worker_node_t
{
public:
    /*!---------------------------------------------------------
     * hbx_tx_band_expert
     *
     * This expert is responsible for setting all the TX RF signal path switches,
     * including switches for the LOs, depending on the desired frequency.
     *
     * ---------------------------------------------------------
     */
    hbx_tx_band_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        uhd::usrp::hbx_rpc_iface::sptr rpcc,
        std::shared_ptr<hbx_cpld_ctrl> hbx_cpld_ctrl,
        std::shared_ptr<hbx_admv1320_ctrl> admv1320_ctrl)
        : uhd::experts::worker_node_t(fe_path / "hbx_tx_band_expert")
        , _frequency(db, fe_path / "freq" / "desired")
        , _rpcc(rpcc)
        , _cpld(hbx_cpld_ctrl)
        , _admv1320_ctrl(admv1320_ctrl)
        , _desired_lo_frequency(
              db, fe_path / "los" / HBX_LO / "freq" / "value" / "desired")
        , _coerced_frequency(db, fe_path / "freq" / "coerced")
        , _desired_rfdc_frequency(
              db, fe_path / "los" / RFDC_NCO / "freq" / "value" / "desired")
    {
        bind_accessor(_frequency);
        bind_accessor(_desired_lo_frequency);
        bind_accessor(_coerced_frequency);
        bind_accessor(_desired_rfdc_frequency);
    }

private:
    void resolve() override;

    // Inputs from Frequency RFDC Freq expert
    uhd::experts::data_reader_t<double> _frequency;

    // RPC interface for controlling the hardware
    uhd::usrp::hbx_rpc_iface::sptr _rpcc;

    // Expects constructed control objects
    std::shared_ptr<hbx_cpld_ctrl> _cpld;

    // ADMV control
    std::shared_ptr<hbx_admv1320_ctrl> _admv1320_ctrl;

    // Output:
    uhd::experts::data_writer_t<double> _desired_lo_frequency;
    uhd::experts::data_writer_t<double> _coerced_frequency;
    uhd::experts::data_writer_t<double> _desired_rfdc_frequency;
};

class hbx_rx_band_expert : public uhd::experts::worker_node_t
{
public:
    /*!---------------------------------------------------------
     * hbx_rx_band_expert
     *
     * This expert is responsible for setting all the RX RF signal path switches,
     * including switches for the LOs, depending on the desired frequency.
     *
     * ---------------------------------------------------------
     */
    hbx_rx_band_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        uhd::usrp::hbx_rpc_iface::sptr rpcc,
        std::shared_ptr<hbx_cpld_ctrl> hbx_cpld_ctrl,
        std::shared_ptr<hbx_demod_ctrl> demod_ctrl,
        std::shared_ptr<hbx_admv1420_ctrl> admv1420_ctrl)
        : uhd::experts::worker_node_t(fe_path / "hbx_rx_band_expert")
        , _frequency(db, fe_path / "freq" / "desired")
        , _rpcc(rpcc)
        , _cpld(hbx_cpld_ctrl)
        , _ltc(demod_ctrl)
        , _admv1420_ctrl(admv1420_ctrl)
        , _desired_lo_frequency(
              db, fe_path / "los" / HBX_LO / "freq" / "value" / "desired")
        , _coerced_frequency(db, fe_path / "freq" / "coerced")
        , _desired_rfdc_frequency(
              db, fe_path / "los" / RFDC_NCO / "freq" / "value" / "desired")
    {
        bind_accessor(_frequency);
        bind_accessor(_desired_lo_frequency);
        bind_accessor(_coerced_frequency);
        bind_accessor(_desired_rfdc_frequency);
    }

private:
    void resolve() override;

    // Inputs from Frequency RFDC Freq expert
    uhd::experts::data_reader_t<double> _frequency;

    // RPC interface for controlling the hardware
    uhd::usrp::hbx_rpc_iface::sptr _rpcc;

    // Expects constructed control objects
    std::shared_ptr<hbx_cpld_ctrl> _cpld;

    // LTC control (demodulator control)
    std::shared_ptr<hbx_demod_ctrl> _ltc;

    // ADMV control
    std::shared_ptr<hbx_admv1420_ctrl> _admv1420_ctrl;

    // Output:
    uhd::experts::data_writer_t<double> _desired_lo_frequency;
    uhd::experts::data_writer_t<double> _coerced_frequency;
    uhd::experts::data_writer_t<double> _desired_rfdc_frequency;
};

class hbx_iq_dc_correction_expert : public uhd::experts::worker_node_t
{
public:
    /*!---------------------------------------------------------
     * hbx_iq_dc_correction_expert
     *
     * This expert is responsible for handling IQ and DC correction settings.
     * ---------------------------------------------------------
     */
    using poke_fn_type = std::function<void(const uint32_t, const uint32_t)>;
    using peek_fn_type = std::function<uint32_t(const uint32_t)>;

    hbx_iq_dc_correction_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path fe_path,
        const uhd::direction_t trx,
        const std::complex<double> dc_conv_offset,
        poke_fn_type&& poke_fn,
        peek_fn_type&& peek_fn)
        : uhd::experts::worker_node_t(fe_path / "hbx_iq_dc_correction_expert")
        , _trx(trx)
        , _peek32(std::move(peek_fn))
        , _poke32(std::move(poke_fn))
        , _rfdc_dc_conv_offset(dc_conv_offset)
        , _iq_offset_base(
              _trx == TX_DIRECTION ? IQ_IMPAIRMENTS_TX_OFFSET : IQ_IMPAIRMENTS_RX_OFFSET)
        , _dc_offset_base(_trx == TX_DIRECTION ? DC_TX_OFFSET : DC_RX_OFFSET)
        , _num_iq_coeffs(_peek32(_iq_offset_base + NUM_COEFFS_REG_OFFSET))
        , _coeffs(db, fe_path / "iq_balance" / "coeffs" / "value" / "desired")
        , _freq(db, fe_path / "freq" / "coerced")
    {
        bind_accessor(_coeffs);
        bind_accessor(_freq);
        // Enable usage of DC offset compensation
        _poke32(_dc_offset_base + DC_CTRL_REG_OFFSET, 1);
    }

private:
    void resolve() override;
    // Helper functions to convert from human-readable formats to poke-able formats.
    // IQ imbalance coefficients to fixed point format.
    uint32_t _coeff_to_fixed(const double coeff) const;
    // DC offset in complex form to fixed point format for I and Q.
    uint32_t _iq_to_dc_offset(const std::complex<double>& cplx_offset) const;
    const uhd::direction_t _trx;
    // Peek and poke methods
    peek_fn_type _peek32;
    poke_fn_type _poke32;
    // Constants that won't change per call to resolve
    const std::complex<double> _rfdc_dc_conv_offset;
    const uint32_t _iq_offset_base;
    const uint32_t _dc_offset_base;
    const uint32_t _num_iq_coeffs;
    // Inputs from user/API
    uhd::experts::data_reader_t<iq_dc_cal_coeffs_t> _coeffs;
    uhd::experts::data_reader_t<double> _freq;
};

using namespace uhd::rfnoc::x400;
using uhd::rfnoc::x400::rfdc_control;
/*!---------------------------------------------------------
 * hbx_sync_expert
 *
 * This expert is responsible for handling the phase alignment.
 * Per channel, there are up to 2 things whose phase need syncing: The NCO, and the
 * ADC/DAC gearboxes. However, the NCOs share a sync register. To minimize writes, we
 * thus need a single sync expert at the end of the graph, which combines all NCOs.
 * --------------------------------------------------------
 */
class hbx_sync_expert : public uhd::experts::worker_node_t
{
public:
    hbx_sync_expert(const uhd::experts::node_retriever_t& db,
        const uhd::fs_path tx_fe_path,
        const uhd::fs_path rx_fe_path,
        rfdc_control::sptr rfdcc)
        : uhd::experts::worker_node_t("hbx_sync_expert")
        , _fe_time(db, rx_fe_path / 0 / "time/fe")
        , _nco_freqs(get_nco_freqs(db, rx_fe_path, tx_fe_path))
        , _rfdcc(rfdcc)
    {
        bind_accessor(_fe_time);

        for (auto& nco_freq : _nco_freqs) {
            bind_accessor(nco_freq.second);
        }
    }

private:
    /*
     * Helper function to get the right number of _fe_time elements depending on the
     * number of channels that are currently active.
     */
    std::vector<uhd::experts::data_reader_t<uhd::time_spec_t>> get_fe_time(
        const uhd::experts::node_retriever_t& db, const uhd::fs_path rx_fe_path)
    {
        std::vector<uhd::experts::data_reader_t<uhd::time_spec_t>> fe_time;
        fe_time.reserve(HBX_MAX_NUM_CHANS);
        fe_time.emplace_back(db, rx_fe_path / 0 / "time/fe");
        return fe_time;
    }

    /*
     * Helper function to get the right number of _nco_freqs elements depending on the
     * number of channels that are currently active.
     */
    std::map<rfdc_control::rfdc_type, uhd::experts::data_reader_t<double>> get_nco_freqs(
        const uhd::experts::node_retriever_t& db,
        const uhd::fs_path rx_fe_path,
        const uhd::fs_path tx_fe_path)
    {
        std::map<rfdc_control::rfdc_type, uhd::experts::data_reader_t<double>> nco_freqs;

        nco_freqs.insert({*RX_RFDC.begin(),
            {db, rx_fe_path / 0 / "los" / RFDC_NCO / "freq" / "value" / "coerced"}});
        nco_freqs.insert({*TX_RFDC.begin(),
            {db, tx_fe_path / 0 / "los" / RFDC_NCO / "freq" / "value" / "coerced"}});

        return nco_freqs;
    }

    void resolve() override;

    // Inputs from user/API
    // Command time: We have up to 4 channels, one time spec per channel
    uhd::experts::data_reader_t<time_spec_t> _fe_time;
    // We have 8 NCOs
    std::map<rfdc_control::rfdc_type, uhd::experts::data_reader_t<double>> _nco_freqs;

    // This expert has no outputs.

    // Attributes
    rfdc_control::sptr _rfdcc;
    //! Store the sync state of the ADC gearboxes. If false, we assume they're
    // out of sync. This could also be a vector of booleans if we want to be
    // able to sync ADC gearboxes individually.
    bool _adcs_synced = false;
    //! Store the sync state of the DAC gearboxes. If false, we assume they're
    // out of sync. This could also be a vector of booleans if we want to be
    // able to sync DAC gearboxes individually.
    bool _dacs_synced = false;
};
}}} // namespace uhd::usrp::hbx

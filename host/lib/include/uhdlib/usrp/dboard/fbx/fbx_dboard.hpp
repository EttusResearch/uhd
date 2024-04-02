//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "fbx_constants.hpp"
#include "fbx_ctrl.hpp"
#include "fbx_expert.hpp"
#include <uhd/exception.hpp>
#include <uhd/experts/expert_factory.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/rfnoc/rf_control/antenna_iface.hpp>
#include <uhd/rfnoc/rf_control/nameless_gain_mixin.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhdlib/rfnoc/rf_control/dboard_iface.hpp>
#include <uhdlib/usrp/common/mpmd_mb_controller.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/usrp/common/x400_rfdc_control.hpp>
#include <uhdlib/usrp/dboard/x400_dboard_iface.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <string>

using namespace uhd::rfnoc;

namespace uhd { namespace usrp { namespace fbx {

const static uint16_t FBX_PID = 0x4007;

/*! \brief Implementation of dboard_iface for balun coupled daughterboard
 */
class fbx_dboard_impl : public uhd::usrp::x400::x400_dboard_iface,
                        public uhd::rfnoc::rf_control::antenna_radio_control_mixin,
                        public uhd::rfnoc::rf_control::nameless_gain_mixin
{
public:
    using sptr                  = std::shared_ptr<fbx_dboard_impl>;
    using time_accessor_fn_type = std::function<uhd::time_spec_t(size_t)>;

    /***********************************************************************
     * Structors
     **********************************************************************/
    fbx_dboard_impl(register_iface& reg_iface,
        const size_t reg_base_address,
        time_accessor_fn_type&& time_accessor,
        const size_t db_idx,
        const std::string& radio_slot,
        const size_t num_tx_chans,
        const size_t num_rx_chans,
        const std::string& rpc_prefix,
        const std::string& unique_id,
        uhd::usrp::x400_rpc_iface::sptr mb_rpcc,
        uhd::usrp::fbx_rpc_iface::sptr rpcc,
        uhd::rfnoc::x400::rfdc_control::sptr rfdcc,
        uhd::property_tree::sptr tree);
    virtual ~fbx_dboard_impl();

    /************************************************************************
     * node_t && noc_block_base API calls
     ***********************************************************************/
    void deinit();

    void set_command_time(uhd::time_spec_t time, const size_t chan) override;

    /************************************************************************
     * API calls
     ***********************************************************************/

    bool is_adc_self_cal_supported() override
    {
        return true;
    }

    uhd::usrp::x400::adc_self_cal_params_t get_adc_self_cal_params(const double) override
    {
        // FBX uses a fixed low freq for both TX and RX
        return {
            // Some conditions were considered for choosing the correct
            // cal_freq, mainly using the PG269 manual as reference. These
            // conditions ensure that the cal_freq or its harmonics don't
            // interfere with the background (BG) calibration mechanism on
            // the RFSoC.
            //
            // 1. In calib_mode2 the highest supported frequency is
            //    0.4 * converter rate (fc).
            //    The minimum fc that X440 supports is 1GHz. So our highest
            //    cal_freq is 400MHz.
            // 2. We need to choose as high a cal_freq as we can
            // 3. The converter rate (fc) should not be a multiple of the cal_freq
            // 4. The converter rate (fc) / 8 should not be a multiple the cal_freq
            // 5. cal_freq should not be of a form k * (fc / 1024) where k = 1 to 1024
            // 6. The specified hysteresis threshold values work with the chosen
            //    cal_freq
            397.55e6, // rx_freq
            397.55e6, // tx_freq
            {0x7FFF, 0}, // output full scale dac mux
            100, // delay
            // From PG.269: "Threshold levels are set as 14-bit unsigned values,
            // with any value from 0 to 16383 allowed. The maximum value, 16383
            // represents the absolute value of the full-scale input of the
            // RF-ADC."
            //
            // X440 in loopback will usually receive a value of ~-11 dBm which
            // translates to a threshold value of ~4000. The minimum value for
            // a useful calibration is -40 dBFS according to Xilinx (~-46 dBm).
            // So we pick a value in between (-20 dBm) to detect if anything is
            // wrong in the signal path and translate this into the 14 bit dBm
            // threshold value which is ~1465. The 'under' value just needs to be
            // slightly lower. Calculation from P_dBm to 14 bit threshold_value:
            //
            // P_rms = math.pow(10,(P_dBm-30)/10)
            // u_peak_to_peak = 2 * math.sqrt(P_rms * 100 * 2) # 100 Ohm Differential
            // # 14 bits threshold, full scale of ADC 1 Vppd ≙ 1 dBm (DS.926):
            // threshold_value = u_peak_to_peak * math.pow(2, 14)
            1365, // under
            1465, // over
            "calib_mode2",
            2000, // 2 seconds were found to be sufficient
        };
    }

    bool select_adc_self_cal_gain(size_t) override;

    double get_converter_rate() const override
    {
        return _rfdc_rate;
    }

    size_t get_num_rx_channels() const override
    {
        return _num_rx_chans;
    }
    size_t get_num_tx_channels() const override
    {
        return _num_tx_chans;
    }

    std::string get_rx_antenna(const size_t chan) const override;

    rf_control::gain_profile_iface::sptr get_tx_gain_profile_api() override
    {
        return _tx_gain_profile_api;
    }

    rf_control::gain_profile_iface::sptr get_rx_gain_profile_api() override
    {
        return _rx_gain_profile_api;
    }

    double set_tx_frequency(const double freq, const size_t chan) override;
    double set_rx_frequency(const double freq, const size_t chan) override;
    uhd::freq_range_t get_tx_frequency_range(const size_t /*chan*/) const override
    {
        return FBX_FREQ_RANGE;
    }
    uhd::freq_range_t get_rx_frequency_range(const size_t /*chan*/) const override
    {
        return FBX_FREQ_RANGE;
    }

    double set_tx_bandwidth(const double bandwidth, const size_t chan) override;
    double set_rx_bandwidth(const double bandwidth, const size_t chan) override;
    uhd::meta_range_t get_tx_bandwidth_range(size_t chan) const override
    {
        return _tree
            ->access<uhd::meta_range_t>(
                _get_frontend_path(TX_DIRECTION, chan) / "bandwidth" / "range")
            .get();
    }
    uhd::meta_range_t get_rx_bandwidth_range(size_t chan) const override
    {
        return _tree
            ->access<uhd::meta_range_t>(
                _get_frontend_path(RX_DIRECTION, chan) / "bandwidth" / "range")
            .get();
    }

    using core_iface::get_rx_gain;
    using core_iface::get_rx_gain_range;
    using core_iface::get_tx_gain;
    using core_iface::get_tx_gain_range;
    using core_iface::set_rx_gain;
    using core_iface::set_tx_gain;

    double set_tx_gain(const double, const std::string&, const size_t) override
    {
        return 0.0;
    }
    double set_rx_gain(const double, const std::string&, const size_t) override
    {
        return 0.0;
    }
    double get_rx_gain(const std::string&, const size_t) override
    {
        return 0.0;
    }
    double get_tx_gain(const std::string&, const size_t) override
    {
        return 0.0;
    }

    // LO Property Getters
    std::vector<std::string> get_tx_lo_names(const size_t /*chan*/) const override
    {
        return {RFDC_NCO};
    }
    std::vector<std::string> get_rx_lo_names(const size_t /*chan*/) const override
    {
        return {RFDC_NCO};
    }
    std::vector<std::string> get_tx_lo_sources(
        const std::string& /*name*/, const size_t /*chan*/) const override
    {
        return {"internal"};
    }
    std::vector<std::string> get_rx_lo_sources(
        const std::string& /*name*/, const size_t /*chan*/) const override
    {
        return {"internal"};
    }

    // LO Frequency Control
    double set_tx_lo_freq(const double, const std::string&, const size_t) override;

    double set_rx_lo_freq(const double, const std::string&, const size_t) override;

    double get_tx_lo_freq(const std::string&, const size_t) override;

    double get_rx_lo_freq(const std::string&, size_t) override;

    // LO Source Control
    void set_tx_lo_source(
        const std::string& /*src*/, const std::string& name, const size_t) override
    {
        _validate_lo_name(name, "set_tx_lo_source");
    }

    void set_rx_lo_source(
        const std::string& /*src*/, const std::string& name, const size_t) override
    {
        _validate_lo_name(name, "set_rx_lo_source");
    }

    const std::string get_tx_lo_source(const std::string&, const size_t) override
    {
        return "internal";
    }

    const std::string get_rx_lo_source(const std::string&, const size_t) override
    {
        return "internal";
    }

    uhd::freq_range_t get_rx_lo_freq_range(
        const std::string& name, const size_t chan) const override
    {
        return _get_lo_freq_range(name, chan);
    }

    uhd::freq_range_t get_tx_lo_freq_range(
        const std::string& name, const size_t chan) override
    {
        return _get_lo_freq_range(name, chan);
    }

    void set_rx_lo_export_enabled(const bool, const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "set_rx_lo_export_enabled is not supported on this radio");
    }
    bool get_rx_lo_export_enabled(const std::string&, const size_t) override
    {
        return false;
    }
    void set_tx_lo_export_enabled(const bool, const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "set_tx_lo_export_enabled is not supported on this radio");
    }
    bool get_tx_lo_export_enabled(const std::string&, const size_t) override
    {
        return false;
    }

    /******************************************************************************
     * EEPROM API
     *****************************************************************************/
    eeprom_map_t get_db_eeprom() override;

    /**************************************************************************
     * Radio Identification API Calls
     *************************************************************************/

    double get_tx_frequency(size_t chan) override;
    double get_rx_frequency(size_t chan) override;
    double get_rx_bandwidth(size_t chan) override;
    double get_tx_bandwidth(size_t chan) override;

    void set_tx_tune_args(const device_addr_t&, const size_t) override
    {
        RFNOC_LOG_TRACE("tune_args not supported by this radio.");
    }

    void set_rx_tune_args(const device_addr_t&, const size_t) override
    {
        RFNOC_LOG_TRACE("tune_args not supported by this radio.");
    }

    std::vector<std::string> get_tx_gain_names(const size_t) const override
    {
        return {};
    }

    uhd::gain_range_t get_tx_gain_range(
        const std::string& /*name*/, const size_t /*chan*/) const override
    {
        return gain_range_t{0.0, 0.0};
    }

    uhd::gain_range_t get_rx_gain_range(
        const std::string& /*name*/, const size_t /*chan*/) const override
    {
        return gain_range_t{0.0, 0.0};
    }

    std::vector<std::string> get_rx_gain_names(const size_t) const override
    {
        return {};
    }

    void set_rx_agc(const bool, const size_t) override
    {
        throw uhd::not_implemented_error("set_rx_agc is not supported on this radio");
    }

    size_t get_chan_from_dboard_fe(const std::string& fe, direction_t) const override;

    std::string get_dboard_fe_from_chan(size_t chan, direction_t) const override;

    std::vector<usrp::pwr_cal_mgr::sptr>& get_pwr_mgr(direction_t) override
    {
        static std::vector<usrp::pwr_cal_mgr::sptr> empty_vtr;
        return empty_vtr;
    }

    /**************************************************************************
     * Helpers
     *************************************************************************/

    std::shared_ptr<fbx_ctrl> get_fbx_ctrl()
    {
        return _fbx_ctrl;
    }

private:
    uhd::property_tree::sptr get_tree()
    {
        return _tree;
    }

    //! Get subtree path for a given direction/channel
    fs_path _get_frontend_path(const direction_t dir, const size_t chan_idx) const;

    // Expert map, keyed by the pair of tx/rx and channel
    uhd::experts::expert_container::sptr _expert_container;

    /**************************************************************************
     * Helpers
     *************************************************************************/
    //! Initialize FBX control
    void _init_fbx_ctrl();

    //! Init a subtree for the RF frontends
    void _init_frontend_subtree(uhd::property_tree::sptr subtree,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);

    //! Initialize property tree
    void _init_prop_tree();
    //! Init RPC interaction
    void _init_mpm();

    //! Initializing the expert properties
    void _init_frequency_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr expert,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);
    void _init_gain_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr expert,
        const fs_path fe_path);
    void _init_antenna_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr expert,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);
    void _init_lo_prop_tree(uhd::property_tree::sptr subtree,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);
    //! Init all experts, bind to properties created above
    void _init_experts(uhd::experts::expert_container::sptr expert,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);

    std::string get_unique_id() const;

    freq_range_t _get_lo_freq_range(const std::string& name, const size_t chan) const;
    void _validate_lo_name(const std::string& namem, const std::string& caller) const;

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    const std::string _unique_id;

    const size_t _num_tx_chans;
    const size_t _num_rx_chans;

    //! Interface to the registers
    uhd::rfnoc::register_iface& _regs;
    const size_t _reg_base_address;

    //! Interface to get the command time
    time_accessor_fn_type _time_accessor;

    //! Letter representation of the radio we're currently running
    const std::string _radio_slot;

    //! Index of this daughterboard
    const size_t _db_idx;

    // infos about the daughtherboard
    std::vector<std::map<std::string, std::string>> _all_dboard_info;

    //! Prepended for all dboard RPC calls
    const std::string _rpc_prefix;

    //! Reference to the MB controller
    uhd::rfnoc::mpmd_mb_controller::sptr _mb_control;

    //! Reference to wb_iface adapters
    std::vector<uhd::timed_wb_iface::sptr> _wb_ifaces;

    //! Reference to the RPC client
    uhd::usrp::x400_rpc_iface::sptr _mb_rpcc;
    uhd::usrp::fbx_rpc_iface::sptr _rpcc;

    //! Reference to the RFDC controller
    uhd::rfnoc::x400::rfdc_control::sptr _rfdcc;

    //! Reference to FBX CTRL
    std::shared_ptr<fbx_ctrl> _fbx_ctrl;

    //! Reference to this block's subtree
    //
    // It is mutable because _tree->access<>(..).get() is not const, but we
    // need to do just that in some const contexts
    mutable uhd::property_tree::sptr _tree;

    rf_control::gain_profile_iface::sptr _tx_gain_profile_api;
    rf_control::gain_profile_iface::sptr _rx_gain_profile_api;

    //! The sampling rate of the RFdc, in the 1-4 GHz range
    const double _rfdc_rate;
};

}}} // namespace uhd::usrp::fbx

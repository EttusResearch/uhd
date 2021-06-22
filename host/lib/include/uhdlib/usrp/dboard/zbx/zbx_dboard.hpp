//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "zbx_constants.hpp"
#include "zbx_cpld_ctrl.hpp"
#include "zbx_expert.hpp"
#include "zbx_lo_ctrl.hpp"
#include <uhd/cal/dsa_cal.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/register_iface.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/wb_iface.hpp>
#include <uhdlib/experts/expert_factory.hpp>
#include <uhdlib/rfnoc/rf_control/dboard_iface.hpp>
#include <uhdlib/usrp/common/mpmd_mb_controller.hpp>
#include <uhdlib/usrp/common/pwr_cal_mgr.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/usrp/common/x400_rfdc_control.hpp>
#include <uhdlib/usrp/dboard/x400_dboard_iface.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <stddef.h>
#include <memory>
#include <string>
#include <vector>

using namespace uhd::rfnoc;

namespace uhd { namespace usrp { namespace zbx {

const static uint16_t ZBX_PID = 0x4002;

/*! Provide access to a ZBX radio.
 */
class zbx_dboard_impl : public uhd::usrp::x400::x400_dboard_iface
{
public:
    using sptr                  = std::shared_ptr<zbx_dboard_impl>;
    using time_accessor_fn_type = std::function<uhd::time_spec_t(size_t)>;

    /************************************************************************
     * Structors
     ***********************************************************************/
    zbx_dboard_impl(register_iface& reg_iface,
        const size_t reg_base_address,
        time_accessor_fn_type&& time_accessor,
        const size_t db_idx,
        const std::string& radio_slot,
        const std::string& rpc_prefix,
        const std::string& unique_id,
        uhd::usrp::x400_rpc_iface::sptr mb_rpcc,
        uhd::usrp::zbx_rpc_iface::sptr rpcc,
        uhd::rfnoc::x400::rfdc_control::sptr rfdcc,
        uhd::property_tree::sptr tree);
    virtual ~zbx_dboard_impl();

    size_t get_chan_from_dboard_fe(
        const std::string& fe, const uhd::direction_t) const override;
    std::string get_dboard_fe_from_chan(
        const size_t chan, const uhd::direction_t) const override;

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

    uhd::usrp::x400::adc_self_cal_params_t get_adc_self_cal_params(const double tone_freq) override
    {
        // This is chosen such that the IF2 frequency is 1.06G
        const double rx_freq = 4.7e9 - 5.12e6;
        const double if2_freq = 1.06e9;
        const double offset = tone_freq - if2_freq;

        // Minus because this zone is inverted
        const double tx_freq = rx_freq - offset;
        return {
            10.0, // min_gain
            50.0, // max_gain
            rx_freq, // rx_freq
            tx_freq, // tx_freq
        };
    }

    rf_control::gain_profile_iface::sptr get_tx_gain_profile_api() override
    {
        return _tx_gain_profile_api;
    }
    rf_control::gain_profile_iface::sptr get_rx_gain_profile_api() override
    {
        return _rx_gain_profile_api;
    }

    void set_tx_antenna(const std::string& ant, const size_t chan) override;
    void set_rx_antenna(const std::string& ant, const size_t chan) override;
    std::vector<std::string> get_tx_antennas(const size_t /*chan*/) const override
    {
        return TX_ANTENNAS;
    }
    std::vector<std::string> get_rx_antennas(const size_t /*chan*/) const override
    {
        return RX_ANTENNAS;
    }

    double set_tx_frequency(const double freq, const size_t chan) override;
    double set_rx_frequency(const double freq, const size_t chan) override;
    uhd::freq_range_t get_tx_frequency_range(const size_t /*chan*/) const override
    {
        return ZBX_FREQ_RANGE;
    }
    uhd::freq_range_t get_rx_frequency_range(const size_t /*chan*/) const override
    {
        return ZBX_FREQ_RANGE;
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

    double set_tx_gain(const double gain, const size_t chan) override;
    double set_tx_gain(
        const double gain, const std::string& name, const size_t chan) override;
    double set_rx_gain(const double gain, const size_t chan) override;
    double set_rx_gain(
        const double gain, const std::string& name, const size_t chan) override;
    double get_rx_gain(const size_t chan) override;
    double get_tx_gain(const size_t chan) override;
    double get_rx_gain(const std::string& name, const size_t chan) override;
    double get_tx_gain(const std::string& name, const size_t chan) override;

    uhd::gain_range_t get_tx_gain_range(const size_t /*chan*/) const override
    {
        return ZBX_TX_GAIN_RANGE;
    }
    uhd::gain_range_t get_rx_gain_range(const size_t /*chan*/) const override
    {
        // FIXME This should return a ZBX_RX_LOW_FREQ_GAIN_RANGE when freq is
        // low, but this function is const
        return ZBX_RX_GAIN_RANGE;
    }

    // LO Property Getters
    std::vector<std::string> get_tx_lo_names(const size_t /*chan*/) const override
    {
        return ZBX_LOS;
    }
    std::vector<std::string> get_rx_lo_names(const size_t /*chan*/) const override
    {
        return ZBX_LOS;
    }
    std::vector<std::string> get_tx_lo_sources(
        const std::string& /*name*/, const size_t /*chan*/) const override
    {
        return std::vector<std::string>{"internal", "external"};
    }
    std::vector<std::string> get_rx_lo_sources(
        const std::string& /*name*/, const size_t /*chan*/) const override
    {
        return std::vector<std::string>{"internal", "external"};
    }

    // LO Frequency Control
    double set_tx_lo_freq(
        const double freq, const std::string& name, const size_t chan) override;
    double set_rx_lo_freq(
        const double freq, const std::string& name, const size_t chan) override;
    double get_tx_lo_freq(const std::string& name, const size_t chan) override;
    double get_rx_lo_freq(const std::string& name, size_t chan) override;

    // LO Source Control
    void set_tx_lo_source(
        const std::string& src, const std::string& name, const size_t chan) override;
    void set_rx_lo_source(
        const std::string& src, const std::string& name, const size_t chan) override;
    const std::string get_tx_lo_source(
        const std::string& name, const size_t chan) override;
    const std::string get_rx_lo_source(
        const std::string& name, const size_t chan) override;

    uhd::freq_range_t get_rx_lo_freq_range(
        const std::string& name, const size_t chan) const override
    {
        return _get_lo_freq_range(name, chan);
    }

    // TODO: Why is this not const?
    uhd::freq_range_t get_tx_lo_freq_range(
        const std::string& name, const size_t chan) override
    {
        return _get_lo_freq_range(name, chan);
    }

    void set_rx_lo_export_enabled(
        const bool enabled, const std::string& name, const size_t chan) override;
    bool get_rx_lo_export_enabled(
        const std::string& name, const size_t chan) override;
    void set_tx_lo_export_enabled(
        const bool enabled, const std::string& name, const size_t chan) override;
    bool get_tx_lo_export_enabled(const std::string& name, const size_t chan) override;


    /******************************************************************************
     * EEPROM API
     *****************************************************************************/
    eeprom_map_t get_db_eeprom() override;

    /**************************************************************************
     * Radio Identification API Calls
     *************************************************************************/

    std::string get_tx_antenna(size_t chan) const override;
    std::string get_rx_antenna(size_t chan) const override;
    double get_tx_frequency(size_t chan) override;
    double get_rx_frequency(size_t chan) override;
    double get_rx_bandwidth(size_t chan) override;
    double get_tx_bandwidth(size_t chan) override;
    void set_tx_tune_args(const uhd::device_addr_t&, const size_t) override;
    void set_rx_tune_args(const uhd::device_addr_t&, const size_t) override;
    std::vector<std::string> get_tx_gain_names(size_t) const override;
    std::vector<std::string> get_rx_gain_names(size_t) const override;

    uhd::gain_range_t get_tx_gain_range(
        const std::string& name, const size_t chan) const override;

    uhd::gain_range_t get_rx_gain_range(
        const std::string& name, const size_t chan) const override;

    void set_rx_agc(const bool, const size_t) override;

    std::vector<uhd::usrp::pwr_cal_mgr::sptr>& get_pwr_mgr(uhd::direction_t trx) override;

private:
    uhd::property_tree::sptr get_tree()
    {
        return _tree;
    }

    // Expert map, keyed by the pair of tx/rx and channel
    uhd::experts::expert_container::sptr _expert_container;

    /**************************************************************************
     * Helpers
     *************************************************************************/
    //! Initialize DB-CPLD
    void _init_cpld();

    //! Initialize all the peripherals connected to this block
    void _init_peripherals();

    //! Init a subtree for the RF frontends
    void _init_frontend_subtree(uhd::property_tree::sptr subtree,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);
    //! Initializing the expert properties
    void _init_frequency_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr expert,
        const fs_path fe_path);
    void _init_gain_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);
    void _init_antenna_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);
    void _init_programming_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr,
        const fs_path fe_path);
    void _init_lo_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);
    //! Init all experts, bind to properties created above
    void _init_experts(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);

    uhd::usrp::pwr_cal_mgr::sptr _init_power_cal(uhd::property_tree::sptr subtree,
        const uhd::direction_t trx,
        const size_t chan_idx,
        const fs_path fe_path);

    //! Initialize property tree
    void _init_prop_tree();
    //! Init RPC interaction
    void _init_mpm();

    //! Set up sensor property nodes
    void _init_mpm_sensors(const direction_t dir, const size_t chan_idx);

    //! Get subtree path for a given direction/channel
    fs_path _get_frontend_path(const direction_t dir, const size_t chan_idx) const;

    // Get all los "lock status", per enabled && locked individual LOs
    bool _get_all_los_locked(const direction_t dir, const size_t chan);

    const std::string _unique_id;
    std::string get_unique_id() const;

    freq_range_t _get_lo_freq_range(const std::string& name, const size_t chan) const;

    /**************************************************************************
     * Private attributes
     *************************************************************************/

    static constexpr size_t _num_rx_chans = 2;
    static constexpr size_t _num_tx_chans = 2;

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
    uhd::usrp::zbx_rpc_iface::sptr _rpcc;

    //! Reference to the RFDC controller
    uhd::rfnoc::x400::rfdc_control::sptr _rfdcc;

    //! Reference to the CPLD controls
    std::shared_ptr<zbx_cpld_ctrl> _cpld;

    //! Reference to all LO controls
    std::map<zbx_lo_t, std::shared_ptr<zbx_lo_ctrl>> _lo_ctrl_map;

    //! Reference to the TX Cal data
    std::shared_ptr<uhd::usrp::cal::zbx_tx_dsa_cal> _tx_dsa_cal;

    //! Reference to the RX Cal data
    std::shared_ptr<uhd::usrp::cal::zbx_rx_dsa_cal> _rx_dsa_cal;

    //! Reference to this block's subtree
    //
    // It is mutable because _tree->access<>(..).get() is not const, but we
    // need to do just that in some const contexts
    mutable uhd::property_tree::sptr _tree;

    std::vector<uhd::usrp::pwr_cal_mgr::sptr> _rx_pwr_mgr;
    std::vector<uhd::usrp::pwr_cal_mgr::sptr> _tx_pwr_mgr;

    rf_control::gain_profile_iface::sptr _tx_gain_profile_api;
    rf_control::gain_profile_iface::sptr _rx_gain_profile_api;

    //! Store the current RX gain profile
    std::vector<std::string> _rx_gain_profile = {
        ZBX_GAIN_PROFILE_DEFAULT, ZBX_GAIN_PROFILE_DEFAULT};
    //! Store the current TX gain profile
    std::vector<std::string> _tx_gain_profile = {
        ZBX_GAIN_PROFILE_DEFAULT, ZBX_GAIN_PROFILE_DEFAULT};

    //! The sampling rate of the RFdc, typically something close to 3 GHz
    const double _rfdc_rate;

    //! The PLL reference rate, typically something in the 50 - 64 MHz range
    const double _prc_rate;
};

}}} // namespace uhd::usrp::zbx

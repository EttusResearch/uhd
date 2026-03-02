//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "hbx_constants.hpp"
#include "hbx_cpld_ctrl.hpp"
#include "hbx_expert.hpp"
#include "hbx_lo_pd.hpp"
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
#include <uhdlib/usrp/common/pwr_cal_mgr.hpp>
#include <uhdlib/usrp/common/rpc.hpp>
#include <uhdlib/usrp/common/x400_rfdc_control.hpp>
#include <uhdlib/usrp/common/x4xx_ch_modes.hpp>
#include <uhdlib/usrp/dboard/x400_dboard_iface.hpp>
#include <uhdlib/utils/rpc.hpp>
#include <string>

using namespace uhd::rfnoc;
using uhd::usrp::x400::ch_mode;

namespace uhd { namespace usrp { namespace hbx {

/*! \brief Implementation of dboard_iface for HBX daughterboard
 */
class hbx_dboard_impl : public uhd::usrp::x400::x400_dboard_iface,
                        public uhd::rfnoc::rf_control::antenna_radio_control_mixin,
                        public uhd::rfnoc::rf_control::nameless_gain_mixin
{
public:
    using sptr                  = std::shared_ptr<hbx_dboard_impl>;
    using time_accessor_fn_type = std::function<uhd::time_spec_t(size_t)>;

    /***********************************************************************
     * Structors
     **********************************************************************/
    hbx_dboard_impl(register_iface& reg_iface,
        const size_t reg_base_address,
        time_accessor_fn_type&& time_accessor,
        const size_t db_idx,
        const std::string& radio_slot,
        const std::string& rpc_prefix,
        const std::string& unique_id,
        uhd::usrp::x400_rpc_iface::sptr mb_rpcc,
        uhd::usrp::hbx_rpc_iface::sptr rpcc,
        uhd::rfnoc::x400::rfdc_control::sptr rfdcc,
        uhd::property_tree::sptr tree,
        const bool ignore_cal_file,
        const double mcr);
    virtual ~hbx_dboard_impl();

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

    uhd::usrp::x400::adc_self_cal_params_t get_adc_self_cal_params() override
    {
        return {
            {0x3FFF, 0}, // output half scale dac mux
            100, // delay
            // Using the values that worked well for X410 (targeting -6 dBFS)
            8000, // under
            8192, //  over, Set the threshold to detect half-scale
                  //  setup_threshold call uses 14-bit ADC values
            "calib_mode2",
            2000, // 2 seconds were found to be sufficient
        };
    }

    uhd::usrp::x400::adc_self_cal_freqs_t get_adc_self_cal_freqs(
        uhd::usrp::x400::ch_mode ch_mode) override
    {
        switch (ch_mode) {
            case ch_mode::REAL:
                return {397.55e6, 397.55e6, uhd::usrp::x400::custom_freq_t::DISALLOW};
            case ch_mode::IQ:
                return {1000e6, 1397.55e6, uhd::usrp::x400::custom_freq_t::DISALLOW};
            default:
                throw uhd::runtime_error(
                    "Invalid ch_mode " + std::to_string(static_cast<int>(ch_mode)));
        };
    }

    bool select_adc_self_cal_gain(size_t chan, size_t mode) override;

    void setup_adc_self_cal() override;

    void finalize_adc_self_cal() override;

    std::vector<ch_mode> get_ch_modes() const override
    {
        return HBX_CH_MODES;
    }

    double get_converter_rate() const override
    {
        return _rfdc_rate;
    }

    size_t get_num_rx_channels() const override
    {
        return HBX_MAX_NUM_CHANS;
    }
    size_t get_num_tx_channels() const override
    {
        return HBX_MAX_NUM_CHANS;
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
        return HBX_FREQ_RANGE;
    }
    uhd::freq_range_t get_rx_frequency_range(const size_t /*chan*/) const override
    {
        return HBX_FREQ_RANGE;
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

    double set_tx_gain(const double gain, const std::string& name, const size_t) final;

    double set_rx_gain(const double gain, const std::string& name, const size_t) final;

    double get_rx_gain(const std::string& name, const size_t) final;

    double get_tx_gain(const std::string& name, const size_t) final;

    // LO Property Getters
    std::vector<std::string> get_tx_lo_names(const size_t /*chan*/) const override
    {
        return HBX_LOS;
    }
    std::vector<std::string> get_rx_lo_names(const size_t /*chan*/) const override
    {
        return HBX_LOS;
    }
    std::vector<std::string> get_tx_lo_sources(
        const std::string& name, const size_t /*chan*/) const override
    {
        if (name == RFDC_NCO) {
            return LO_SOURCE_INT;
        }
        return LO_SOURCE_ALL;
    }
    std::vector<std::string> get_rx_lo_sources(
        const std::string& name, const size_t /*chan*/) const override
    {
        if (name == RFDC_NCO) {
            return LO_SOURCE_INT;
        }
        return LO_SOURCE_ALL;
    }

    // LO Frequency Control
    double set_tx_lo_freq(
        const double freq, const std::string& name, const size_t) override;
    double set_rx_lo_freq(
        const double freq, const std::string& name, const size_t) override;
    double get_tx_lo_freq(const std::string& name, const size_t) override;
    double get_rx_lo_freq(const std::string& name, const size_t) override;

    // LO Source Control
    void set_tx_lo_source(
        const std::string& src, const std::string& name, const size_t /*ch*/) final;

    void set_rx_lo_source(
        const std::string& src, const std::string& name, const size_t /*ch*/) final;

    std::string get_tx_lo_source(const std::string& name, const size_t) final;

    std::string get_rx_lo_source(const std::string& name, const size_t) final;

    uhd::freq_range_t get_rx_lo_freq_range(
        const std::string& name, const size_t /*chan*/) const override
    {
        return _get_lo_freq_range(name);
    }

    uhd::freq_range_t get_tx_lo_freq_range(
        const std::string& name, const size_t /*chan*/) override
    {
        return _get_lo_freq_range(name);
    }

    void set_rx_lo_export_enabled(
        const bool enabled, const std::string& name, const size_t) final;

    bool get_rx_lo_export_enabled(const std::string& name, const size_t) final;

    void set_tx_lo_export_enabled(
        const bool enabled, const std::string& name, const size_t) final;

    bool get_tx_lo_export_enabled(const std::string& name, const size_t) final;

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

    std::vector<std::string> get_tx_gain_names(const size_t) const final;

    uhd::gain_range_t get_tx_gain_range(
        const std::string& /*name*/, const size_t /*chan*/) const final;

    uhd::gain_range_t get_rx_gain_range(
        const std::string& /*name*/, const size_t /*chan*/) const final;

    std::vector<std::string> get_rx_gain_names(const size_t) const final;

    void set_rx_agc(const bool, const size_t) override
    {
        throw uhd::not_implemented_error("set_rx_agc is not supported on this radio");
    }

    size_t get_chan_from_dboard_fe(const std::string& fe, direction_t) const override;

    std::string get_dboard_fe_from_chan(size_t chan, direction_t dir) const override;

    std::vector<usrp::pwr_cal_mgr::sptr>& get_pwr_mgr(direction_t trx) final;

    /**************************************************************************
     * Helpers
     *************************************************************************/

    //! Get a pointer to the HBX CPLD control
    std::shared_ptr<hbx_cpld_ctrl> get_hbx_cpld_ctrl()
    {
        return _cpld;
    }

private:
    uhd::property_tree::sptr get_tree()
    {
        return _tree;
    }

    //! Get subtree path for a given direction
    fs_path _get_frontend_path(const direction_t dir, const size_t chan_idx) const;

    //! Check if given LO name is valid and translate from GENERIC_LO to actual LO name if
    //! needed
    std::string _validate_lo_name(const std::string& name) const;


    // Expert map, keyed by the pair of tx/rx
    uhd::experts::expert_container::sptr _expert_container;

    /**************************************************************************
     * Helpers
     *************************************************************************/
    //! Initialize HBX control
    void _init_hbx_cpld();

    //! Initialization of the ADMV chips
    void _init_admv();

    //! Create an LO Ctrl
    std::shared_ptr<hbx_lo_ctrl> _init_lo_ctrl(const direction_t trx);

    //! Initialization of Demod Ctrl
    void _init_demod();

    //! Initialization of LO Power Detector
    std::shared_ptr<hbx_lo_pd> _init_lo_pd(const direction_t trx);

    //! Init a subtree for the RF frontends
    void _init_frontend_subtree(uhd::property_tree::sptr subtree,
        const uhd::direction_t trx,
        const fs_path fe_path);

    //! Initialize property tree
    void _init_prop_tree();

    //! Init RPC interaction
    void _init_mpm();

    //! Initializing the expert properties
    void _init_frequency_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr expert,
        const uhd::direction_t trx,
        const fs_path fe_path);
    void _init_gain_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr expert,
        const uhd::direction_t trx,
        const fs_path fe_path);
    void _init_antenna_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr expert,
        const uhd::direction_t trx,
        const fs_path fe_path);
    void _init_lo_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr expert,
        const uhd::direction_t trx,
        const fs_path fe_path);
    void _init_iq_dc_prop_tree(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr expert,
        const uhd::direction_t trx,
        const fs_path fe_path);

    //! Init all experts, bind to properties created above
    void _init_experts(uhd::property_tree::sptr subtree,
        uhd::experts::expert_container::sptr expert,
        const uhd::direction_t trx,
        const fs_path fe_path);

    uhd::usrp::pwr_cal_mgr::sptr _init_power_cal(uhd::property_tree::sptr subtree,
        const uhd::direction_t trx,
        const fs_path fe_path);

    std::string get_unique_id() const;

    freq_range_t _get_lo_freq_range(const std::string& name) const;
    void _validate_lo_name(const std::string& name, const std::string& caller) const;

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    const std::string _unique_id;

    //! Interface to the registers
    uhd::rfnoc::register_iface& _reg_iface;
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
    uhd::usrp::hbx_rpc_iface::sptr _rpcc;

    //! Reference to the RFDC controller
    uhd::rfnoc::x400::rfdc_control::sptr _rfdcc;

    //! Reference to HBX CPLD CTRL
    std::shared_ptr<hbx_cpld_ctrl> _cpld;

    //! Reference to ADMV1320 CTRL
    std::shared_ptr<hbx_admv1320_ctrl> _admv1320;

    //! Reference to ADMV1420 CTRL
    std::shared_ptr<hbx_admv1420_ctrl> _admv1420;

    //! Reference to the RX LO ctrl
    std::shared_ptr<hbx_lo_ctrl> _rx_lo_ctrl;

    //! Reference to the TX LO ctrl
    std::shared_ptr<hbx_lo_ctrl> _tx_lo_ctrl;

    //! Reference to the demod ctrl
    std::shared_ptr<hbx_demod_ctrl> _demod_ctrl;

    //! Reference to TX LO Detector
    std::shared_ptr<hbx_lo_pd> _tx_lo_pd;

    //! Reference to RX LO Detector
    std::shared_ptr<hbx_lo_pd> _rx_lo_pd;

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
    std::string _rx_gain_profile = HBX_GAIN_PROFILE_DEFAULT;

    //! Store the current TX gain profile
    std::string _tx_gain_profile = HBX_GAIN_PROFILE_DEFAULT;

    //! The sampling rate of the RFdc, in the 1-4 GHz range
    const double _rfdc_rate;

    //! The PLL reference rate, typically something in the 50 - 64 MHz range
    const double _prc_rate;

    const bool _ignore_cal_file;

    std::string _db_serial;

    const double _mcr;

    std::complex<double> _rfdc_dc_offset = {0.0, 0.0};
};

}}} // namespace uhd::usrp::hbx

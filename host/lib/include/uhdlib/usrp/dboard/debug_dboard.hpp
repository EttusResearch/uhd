//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "x400_dboard_iface.hpp"
#include <uhd/exception.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhdlib/rfnoc/rf_control/dboard_iface.hpp>
#include <string>

#define UHD_LOG_SKIP_CFG() \
    UHD_LOG_TRACE(         \
        "RFNOC::DEBUG_DB", "Skipping unsupported debug db config for " << __FUNCTION__);

namespace uhd { namespace rfnoc {

const static uint16_t EMPTY_DB_PID       = 0x0;
const static uint16_t DEBUG_DB_PID       = 0x4001;
const static uint16_t IF_TEST_DBOARD_PID = 0x4006;

/*! \brief Implementation of common dboard_iface for IF Test and Debug dboards.
 */
class debug_dboard_common_impl : public uhd::usrp::x400::x400_dboard_iface
{
public:
    using sptr = std::shared_ptr<debug_dboard_common_impl>;

    rf_control::gain_profile_iface::sptr get_tx_gain_profile_api() override
    {
        return rf_control::gain_profile_iface::sptr();
    }

    rf_control::gain_profile_iface::sptr get_rx_gain_profile_api() override
    {
        return rf_control::gain_profile_iface::sptr();
    }

    bool is_adc_self_cal_supported() override
    {
        return false;
    }

    uhd::usrp::x400::adc_self_cal_params_t get_adc_self_cal_params(double) override
    {
        return {
            0.0,
            0.0,
            0.0,
            0.0,
        };
    }

    size_t get_chan_from_dboard_fe(const std::string& fe, direction_t) const override
    {
        if (fe == "0") {
            return 0;
        }
        if (fe == "1") {
            return 1;
        }
        throw uhd::key_error(std::string("[X400] Invalid frontend: ") + fe);
    }

    std::string get_dboard_fe_from_chan(size_t chan, direction_t) const override
    {
        if (chan == 0) {
            return "0";
        }
        if (chan == 1) {
            return "1";
        }
        throw uhd::lookup_error(
            std::string("[X400] Invalid channel: ") + std::to_string(chan));
    }

    std::vector<usrp::pwr_cal_mgr::sptr>& get_pwr_mgr(direction_t) override
    {
        static std::vector<usrp::pwr_cal_mgr::sptr> empty_vtr;
        return empty_vtr;
    }

    eeprom_map_t get_db_eeprom() override
    {
        return {};
    }

    std::string get_tx_antenna(const size_t) const override
    {
        return "";
    }

    std::vector<std::string> get_tx_antennas(const size_t) const override
    {
        return {};
    }

    void set_tx_antenna(const std::string&, const size_t) override{UHD_LOG_SKIP_CFG()}

    std::string get_rx_antenna(const size_t) const override
    {
        return "";
    }

    std::vector<std::string> get_rx_antennas(const size_t) const override
    {
        return {};
    }

    void set_rx_antenna(const std::string&, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
    }

    double get_tx_frequency(const size_t) override
    {
        return 0;
    }

    double set_tx_frequency(const double, size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    void set_tx_tune_args(const device_addr_t&, const size_t) override{UHD_LOG_SKIP_CFG()}

    freq_range_t get_tx_frequency_range(const size_t) const override
    {
        return meta_range_t(0.0, 0.0);
    }

    double get_rx_frequency(const size_t) override
    {
        return 0;
    }

    double set_rx_frequency(const double, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    void set_rx_tune_args(const device_addr_t&, const size_t) override{UHD_LOG_SKIP_CFG()}

    freq_range_t get_rx_frequency_range(const size_t) const override
    {
        return meta_range_t(0.0, 0.0);
    }

    std::vector<std::string> get_tx_gain_names(const size_t) const override
    {
        return {};
    }

    gain_range_t get_tx_gain_range(const size_t) const override
    {
        return meta_range_t(0.0, 0.0);
    }

    gain_range_t get_tx_gain_range(const std::string&, const size_t) const override
    {
        return meta_range_t(0.0, 0.0);
    }

    double get_tx_gain(const size_t) override
    {
        return 0;
    }

    double get_tx_gain(const std::string&, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    double set_tx_gain(const double, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    double set_tx_gain(const double, const std::string&, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    std::vector<std::string> get_rx_gain_names(const size_t) const override
    {
        return {};
    }

    gain_range_t get_rx_gain_range(const size_t) const override
    {
        UHD_LOG_SKIP_CFG()
        return meta_range_t(0.0, 0.0);
    }

    gain_range_t get_rx_gain_range(const std::string&, const size_t) const override
    {
        UHD_LOG_SKIP_CFG()
        return meta_range_t(0.0, 0.0);
    }

    double get_rx_gain(const size_t) override
    {
        return 0;
    }

    double get_rx_gain(const std::string&, const size_t) override
    {
        return 0;
    }

    double set_rx_gain(const double, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    double set_rx_gain(const double, const std::string&, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    void set_rx_agc(const bool, const size_t) override{UHD_LOG_SKIP_CFG()}

    meta_range_t get_tx_bandwidth_range(size_t) const override
    {
        return meta_range_t(0.0, 0.0);
    }

    double get_tx_bandwidth(const size_t) override
    {
        return 0;
    }

    double set_tx_bandwidth(const double, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    meta_range_t get_rx_bandwidth_range(size_t) const override
    {
        return meta_range_t(0.0, 0.0);
    }

    double get_rx_bandwidth(const size_t) override
    {
        return 0;
    }

    double set_rx_bandwidth(const double, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    std::vector<std::string> get_rx_lo_names(const size_t) const override
    {
        return {};
    }

    std::vector<std::string> get_rx_lo_sources(
        const std::string&, const size_t) const override
    {
        UHD_LOG_SKIP_CFG()
        return {};
    }

    freq_range_t get_rx_lo_freq_range(const std::string&, const size_t) const override
    {
        return meta_range_t(0.0, 0.0);
    }

    void set_rx_lo_source(const std::string&, const std::string&, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
    }

    const std::string get_rx_lo_source(const std::string&, const size_t) override
    {
        return "";
    }

    void set_rx_lo_export_enabled(bool, const std::string&, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
    }

    bool get_rx_lo_export_enabled(const std::string&, const size_t) override
    {
        return false;
    }

    double set_rx_lo_freq(double, const std::string&, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    double get_rx_lo_freq(const std::string&, const size_t) override
    {
        return 0;
    }

    std::vector<std::string> get_tx_lo_names(const size_t) const override
    {
        return {};
    }

    std::vector<std::string> get_tx_lo_sources(const std::string&, const size_t) const override
    {
        return {};
    }

    freq_range_t get_tx_lo_freq_range(const std::string&, const size_t) override
    {
        return meta_range_t(0.0, 0.0);
    }

    void set_tx_lo_source(const std::string&, const std::string&, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
    }

    const std::string get_tx_lo_source(const std::string&, const size_t) override
    {
        return "";
    }

    void set_tx_lo_export_enabled(const bool, const std::string&, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
    }

    bool get_tx_lo_export_enabled(const std::string&, const size_t) override
    {
        return false;
    }

    double set_tx_lo_freq(const double, const std::string&, const size_t) override
    {
        UHD_LOG_SKIP_CFG()
        return 0;
    }

    double get_tx_lo_freq(const std::string&, const size_t) override
    {
        return 0;
    }

    void set_command_time(uhd::time_spec_t, const size_t) override
    {
        // nop
    }
};

/*! \brief Implementation of dboard_iface for debug_db.
 */
class debug_dboard_impl : public debug_dboard_common_impl
{
    // Just an empty class for conveniently organizing class hierarchy.
};

/*! \brief Fake dboard implementation for an empty slot
 */
class empty_slot_dboard_impl : public debug_dboard_common_impl
{
    // Just an empty class for conveniently organizing class hierarchy.
};

/*! \brief Implementation of dboard_iface for IF Test dboard.
 */
class if_test_dboard_impl : public debug_dboard_common_impl
{
public:
    /******************************************************************************
     * Structors
     *****************************************************************************/
    if_test_dboard_impl(const size_t db_idx,
        const std::string& rpc_prefix,
        const std::string& unique_id,
        std::shared_ptr<mpmd_mb_controller> mb_controller,
        uhd::property_tree::sptr tree)
        : _unique_id(unique_id)
        , _db_idx(db_idx)
        , _rpc_prefix(rpc_prefix)
        , _mb_control(mb_controller)
        , _tree(tree)
    {
        RFNOC_LOG_TRACE("Entering " << __FUNCTION__);
        RFNOC_LOG_TRACE("DB ID: " << _db_idx);
        UHD_ASSERT_THROW(_mb_control);
        _rpcc = _mb_control->get_rpc_client();
        UHD_ASSERT_THROW(_rpcc);
        _init_frontend_subtree();
    }

    ~if_test_dboard_impl()
    {
        RFNOC_LOG_TRACE(__FUNCTION__);
    }

    // The IF Test dboard muxes a single SMA port (for each of RX and TX) like so:
    //                        /---> dac0
    //                       /----> dac1
    //  TX SMA port -- [mux] -----> dac2
    //                       \----> dac3
    //
    // (and similarly with the RX SMA port and the adcs)

    std::vector<std::string> get_tx_muxes(void)
    {
        return {"DAC0", "DAC1", "DAC2", "DAC3"};
    }

    void set_tx_mux(const std::string& mux)
    {
        RFNOC_LOG_TRACE("Setting TX mux to " << mux);
        _rpcc->notify_with_token(
            _rpc_prefix + "config_tx_path", _get_tx_path_from_mux(mux));
    }

    std::string get_tx_mux(void)
    {
        return _rpcc->request_with_token<std::string>(_rpc_prefix + "get_tx_path");
    }

    std::vector<std::string> get_rx_muxes(void)
    {
        return {"ADC0", "ADC1", "ADC2", "ADC3"};
    }

    void set_rx_mux(const std::string& mux)
    {
        RFNOC_LOG_TRACE("Setting RX mux to " << mux);
        _rpcc->notify_with_token(
            _rpc_prefix + "config_rx_path", _get_rx_path_from_mux(mux));
    }

    std::string get_rx_mux(void)
    {
        return _rpcc->request_with_token<std::string>(_rpc_prefix + "get_rx_path");
    }

    eeprom_map_t get_db_eeprom() override
    {
        return _rpcc->request_with_token<eeprom_map_t>("get_db_eeprom", _db_idx);
    }


private:
    //! Used by the RFNOC_LOG_* macros.
    const std::string _unique_id;
    std::string get_unique_id() const
    {
        return _unique_id;
    }

    //! Index of this daughterboard
    const size_t _db_idx;

    //! Prepended for all dboard RPC calls
    const std::string _rpc_prefix;

    //! Reference to the MB controller
    uhd::rfnoc::mpmd_mb_controller::sptr _mb_control;

    //! Reference to the RPC client
    uhd::rpc_client::sptr _rpcc;

    //! Reference to this block's subtree
    //
    // It is mutable because _tree->access<>(..).get() is not const, but we
    // need to do just that in some const contexts
    mutable uhd::property_tree::sptr _tree;

    std::string _get_tx_path_from_mux(const std::string mux)
    {
        if (mux == "DAC0") {
            return "dac0";
        } else if (mux == "DAC1") {
            return "dac1";
        } else if (mux == "DAC2") {
            return "dac2";
        } else if (mux == "DAC3") {
            return "dac3";
        } else {
            throw uhd::value_error(
                std::string("[RFNOC::IF_TEST_DBOARD] Invalid TX Mux Name: ") + mux);
        }
    }

    std::string _get_rx_path_from_mux(const std::string mux)
    {
        if (mux == "ADC0") {
            return "adc0";
        } else if (mux == "ADC1") {
            return "adc1";
        } else if (mux == "ADC2") {
            return "adc2";
        } else if (mux == "ADC3") {
            return "adc3";
        } else {
            throw uhd::value_error(
                std::string("[RFNOC::IF_TEST_DBOARD] Invalid RX Mux Name: ") + mux);
        }
    }

    void _init_frontend_subtree()
    {
        auto subtree = _tree->subtree(fs_path("dboard"));

        // DB EEPROM
        subtree->create<eeprom_map_t>("eeprom")
            .add_coerced_subscriber([](const eeprom_map_t&) {
                throw uhd::runtime_error("Attempting to update daughterboard eeprom!");
            })
            .set_publisher([this]() { return get_db_eeprom(); });

        static const char IF_TEST_FE_NAME[] = "IF_TEST";

        const fs_path tx_fe_path = fs_path("tx_frontends/0");
        const fs_path rx_fe_path = fs_path("rx_frontends/0");
        RFNOC_LOG_TRACE("Adding non-RFNoC block properties"
                        << " to prop tree path " << tx_fe_path << " and " << rx_fe_path);

        subtree->create<std::string>(tx_fe_path / "name").set(IF_TEST_FE_NAME);
        subtree->create<std::string>(rx_fe_path / "name").set(IF_TEST_FE_NAME);

        // TX Mux
        subtree->create<std::string>(tx_fe_path / "mux" / "value")
            .add_coerced_subscriber(
                [this](const std::string& mux) { this->set_tx_mux(mux); })
            .set_publisher([this]() { return this->get_tx_mux(); });
        subtree->create<std::vector<std::string>>(tx_fe_path / "mux" / "options")
            .set(get_tx_muxes())
            .add_coerced_subscriber([](const std::vector<std::string>&) {
                throw uhd::runtime_error("Attempting to update mux options!");
            });

        // RX Mux
        subtree->create<std::string>(rx_fe_path / "mux" / "value")
            .add_coerced_subscriber(
                [this](const std::string& mux) { this->set_rx_mux(mux); })
            .set_publisher([this]() { return this->get_rx_mux(); });
        subtree->create<std::vector<std::string>>(rx_fe_path / "mux" / "options")
            .set(get_rx_muxes())
            .add_coerced_subscriber([](const std::vector<std::string>&) {
                throw uhd::runtime_error("Attempting to update mux options!");
            });

        for (auto fe_path : {tx_fe_path, rx_fe_path}) {
            // Antennas
            const std::vector<std::string> antenna_options = {"SMA"};
            subtree->create<std::vector<std::string>>(fe_path / "antenna" / "options")
                .set(antenna_options)
                .add_coerced_subscriber([](const std::vector<std::string>&) {
                    throw uhd::runtime_error("Attempting to update antenna options!");
                });

            // Frequency range
            const uhd::freq_range_t freq_range(0.0, 0.0);
            subtree->create<meta_range_t>(fe_path / "freq" / "range")
                .set(freq_range)
                .add_coerced_subscriber([](const meta_range_t&) {
                    throw uhd::runtime_error("Attempting to update freq range!");
                });

            // Gains
            const uhd::gain_range_t gain_range(0.0, 0.0, 1.0);
            subtree->create<meta_range_t>(fe_path / "gains" / "all" / "range")
                .set(gain_range)
                .add_coerced_subscriber([](const meta_range_t&) {
                    throw uhd::runtime_error("Attempting to update gain range!");
                });

            // Connection
            subtree->create<std::string>(fe_path / "connection").set("IQ");
        }
    }
};

}} // namespace uhd::rfnoc

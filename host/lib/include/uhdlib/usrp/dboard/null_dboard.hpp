//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "x400_dboard_iface.hpp"
#include <uhd/exception.hpp>
#include <uhdlib/rfnoc/rf_control/dboard_iface.hpp>
#include <string>

namespace uhd { namespace rfnoc {

/*! \brief Implementation of dboard_iface for unpopulated or unsupported daughterboards
 */
class null_dboard_impl : public uhd::usrp::x400::x400_dboard_iface
{
public:
    using sptr = std::shared_ptr<null_dboard_impl>;

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

    void set_tx_antenna(const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

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
        throw _no_dboard_exception();
    }

    double get_tx_frequency(const size_t) override
    {
        return 0;
    }

    double set_tx_frequency(const double, size_t) override
    {
        throw _no_dboard_exception();
    }

    void set_tx_tune_args(const device_addr_t&, const size_t) override
    {
        throw _no_dboard_exception();
    }

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
        throw _no_dboard_exception();
    }

    void set_rx_tune_args(const device_addr_t&, const size_t) override
    {
        throw _no_dboard_exception();
    }

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
        throw _no_dboard_exception();
    }

    double set_tx_gain(const double, const size_t) override
    {
        throw _no_dboard_exception();
    }

    double set_tx_gain(const double, const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    std::vector<std::string> get_rx_gain_names(const size_t) const override
    {
        return {};
    }

    gain_range_t get_rx_gain_range(const size_t) const override
    {
        throw _no_dboard_exception();
    }

    gain_range_t get_rx_gain_range(const std::string&, const size_t) const override
    {
        throw _no_dboard_exception();
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
        throw _no_dboard_exception();
    }

    double set_rx_gain(const double, const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    void set_rx_agc(const bool, const size_t) override
    {
        throw _no_dboard_exception();
    }

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
        throw _no_dboard_exception();
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
        throw _no_dboard_exception();
    }

    std::vector<std::string> get_rx_lo_names(const size_t) const override
    {
        return {};
    }

    std::vector<std::string> get_rx_lo_sources(
        const std::string&, const size_t) const override
    {
        throw _no_dboard_exception();
    }

    freq_range_t get_rx_lo_freq_range(const std::string&, const size_t) const override
    {
        throw _no_dboard_exception();
    }

    void set_rx_lo_source(const std::string&, const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    const std::string get_rx_lo_source(const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    void set_rx_lo_export_enabled(bool, const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    bool get_rx_lo_export_enabled(const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    double set_rx_lo_freq(double, const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    double get_rx_lo_freq(const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    std::vector<std::string> get_tx_lo_names(const size_t) const override
    {
        return {};
    }

    std::vector<std::string> get_tx_lo_sources(const std::string&, const size_t) const override
    {
        throw _no_dboard_exception();
    }

    freq_range_t get_tx_lo_freq_range(const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    void set_tx_lo_source(const std::string&, const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    const std::string get_tx_lo_source(const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    void set_tx_lo_export_enabled(const bool, const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    bool get_tx_lo_export_enabled(const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    double set_tx_lo_freq(const double, const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    double get_tx_lo_freq(const std::string&, const size_t) override
    {
        throw _no_dboard_exception();
    }

    void set_command_time(uhd::time_spec_t, const size_t) override
    {
        // nop
    }

private:
    uhd::runtime_error _no_dboard_exception() const
    {
        const std::string msg("No daughterboard or daughterboard with unrecognized PID.");
        return uhd::runtime_error(msg);
    }
};

}} // namespace uhd::rfnoc

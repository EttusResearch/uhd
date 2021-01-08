//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e3xx_ad9361_iface.hpp"
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <boost/format.hpp>

using namespace uhd;

class e3xx_ad9361_iface : public ad9361_ctrl
{
public:
    e3xx_ad9361_iface(rpc_client::sptr rpcc)
        : _rpcc(rpcc), _rpc_prefix("db_0_"), _log_prefix("AD9361")
    {
        UHD_LOG_TRACE(
            _log_prefix, "Initialized controls with RPC prefix " << _rpc_prefix);
    }

    double set_bw_filter(const std::string& which, const double bw) override
    {
        return _rpcc->request_with_token<double>(
            this->_rpc_prefix + "set_bw_filter", which, bw);
    }

    double set_gain(const std::string& which, const double value) override
    {
        return _rpcc->request_with_token<double>(
            this->_rpc_prefix + "set_gain", which, value);
    }

    void set_agc(const std::string& which, bool enable) override
    {
        _rpcc->request_with_token<void>(this->_rpc_prefix + "set_agc", which, enable);
    }

    void set_agc_mode(const std::string& which, const std::string& mode) override
    {
        _rpcc->request_with_token<void>(this->_rpc_prefix + "set_agc_mode", which, mode);
    }

    double set_clock_rate(const double rate) override
    {
        return _rpcc->request_with_token<double>(
            E3XX_RATE_TIMEOUT, this->_rpc_prefix + "set_catalina_clock_rate", rate);
    }

    void set_active_chains(bool tx1, bool tx2, bool rx1, bool rx2) override
    {
        _rpcc->request_with_token<void>(
            this->_rpc_prefix + "set_active_chains", tx1, tx2, rx1, rx2);
    }

    double tune(const std::string& which, const double value) override
    {
        return _rpcc->request_with_token<double>(
            E3XX_TUNE_TIMEOUT, this->_rpc_prefix + "catalina_tune", which, value);
    }

    void set_dc_offset_auto(const std::string& which, const bool on) override
    {
        _rpcc->request_with_token<void>(
            this->_rpc_prefix + "set_dc_offset_auto", which, on);
    }

    void set_timing_mode(const std::string& timing_mode) override
    {
        _rpcc->request_with_token<void>(
            this->_rpc_prefix + "set_timing_mode", timing_mode);
    }

    void set_iq_balance_auto(const std::string& which, const bool on) override
    {
        _rpcc->request_with_token<void>(
            this->_rpc_prefix + "set_iq_balance_auto", which, on);
    }

    double get_freq(const std::string& which) override
    {
        return _rpcc->request_with_token<double>(this->_rpc_prefix + "get_freq", which);
    }

    void data_port_loopback(const bool on) override
    {
        _rpcc->request_with_token<void>(this->_rpc_prefix + "data_port_loopback", on);
    }

    sensor_value_t get_rssi(const std::string& which) override
    {
        return sensor_value_t(_rpcc->request_with_token<sensor_value_t::sensor_map_t>(
            this->_rpc_prefix + "get_rssi", which));
    }

    sensor_value_t get_temperature() override
    {
        return sensor_value_t(_rpcc->request_with_token<sensor_value_t::sensor_map_t>(
            this->_rpc_prefix + "get_temperature"));
    }

    std::vector<std::string> get_filter_names(const std::string& which) override
    {
        return _rpcc->request_with_token<std::vector<std::string>>(
            this->_rpc_prefix + "get_filter_names", which);
    }

    filter_info_base::sptr get_filter(
        const std::string& /*which*/, const std::string& /*filter_name*/) override
    {
        throw uhd::runtime_error(
            "ad9361_ctrl::get_filter is not supported over an RPC connection");
    }

    void set_filter(const std::string& /*which*/,
        const std::string& /*filter_name*/,
        const filter_info_base::sptr /*filter*/) override
    {
        throw uhd::runtime_error(
            "ad9361_ctrl::set_filter is not supported over an RPC connection");
    }

    void output_digital_test_tone(bool enb) override
    {
        _rpcc->request_with_token<void>(
            this->_rpc_prefix + "output_digital_test_tone", enb);
    }

private:
    //! Reference to the RPC client
    rpc_client::sptr _rpcc;

    //! Stores the prefix to RPC calls
    const std::string _rpc_prefix;

    //! Logger prefix
    const std::string _log_prefix;
};

//! Factory function for E3xx's AD9361 RPC Controller
ad9361_ctrl::sptr make_rpc(rpc_client::sptr rpcc)
{
    return ad9361_ctrl::sptr(new e3xx_ad9361_iface(rpcc));
}

/*! Helper function to convert direction and channel to the 'which' required by most
   Catalina driver functions */
std::string get_which_ad9361_chain(
    const direction_t dir, const size_t chan, const bool fe_swap)
{
    UHD_ASSERT_THROW(dir == RX_DIRECTION or dir == TX_DIRECTION);
    UHD_ASSERT_THROW(chan == 0 or chan == 1);
    size_t ad9361_chan = fe_swap ? (chan ? 0 : 1) : chan;
    return str(
        boost::format("%s%d") % (dir == RX_DIRECTION ? "RX" : "TX") % (ad9361_chan + 1));
}

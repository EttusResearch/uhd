//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/experts/expert_factory.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhd/rfnoc/rf_control/antenna_iface.hpp>
#include <uhd/rfnoc/rf_control/nameless_gain_mixin.hpp>
#include <extension_example/extension_example.hpp>
#include <string>
#include <vector>

namespace ext_example {

using namespace uhd;

class extension_example_impl : public extension_example,
                               public uhd::rfnoc::rf_control::antenna_radio_control_mixin,
                               public uhd::rfnoc::rf_control::nameless_gain_mixin
{
public:
    using sptr = std::shared_ptr<extension_example_impl>;

    extension_example_impl(
        uhd::rfnoc::radio_control::sptr radio, uhd::property_tree::sptr tree);

    ~extension_example_impl() = default;


    // This is our custom method that is specific to the example extension. It was
    // declared in the extension_example.hpp under include/.
    void write_log(std::string& text) override
    {
        UHD_LOG_INFO("EXTENSION_EXAMPLE", text);
    }

    // The extension may just get values from the radio or it can implement its own logic.
    double get_tx_frequency(const size_t chan) override
    {
        return _radio->get_tx_frequency(chan);
    }

    // For the sake of demonstration let the extension manipulate the frequency (multiply
    // by 2)
    double set_tx_frequency(const double freq, size_t chan) override
    {
        return _radio->set_tx_frequency(2.0 * freq, chan);
    }

    void set_tx_tune_args(const device_addr_t& args, const size_t chan) override
    {
        _radio->set_tx_tune_args(args, chan);
    }

    // Let the implementation do more advanced functionality
    freq_range_t get_tx_frequency_range(const size_t) const override;

    double get_rx_frequency(const size_t chan) override
    {
        return _radio->get_rx_frequency(chan);
    }

    double set_rx_frequency(const double freq, const size_t chan) override
    {
        return _radio->set_rx_frequency(freq, chan);
    }

    void set_rx_tune_args(const device_addr_t& args, const size_t chan) override
    {
        _radio->set_rx_tune_args(args, chan);
    }

    freq_range_t get_rx_frequency_range(const size_t) const override;

    std::vector<std::string> get_tx_gain_names(const size_t) const override
    {
        return {"all"};
    }

    gain_range_t get_tx_gain_range(
        const std::string& name, const size_t chan) const override
    {
        return _radio->get_tx_gain_range(name, chan);
    }

    // This demonstrates the usage of the experts: Here we write the gain into the
    // property tree. In extension_example.cpp this change will be received by an expert
    // which will then call its resolve method which can manipulate the data.
    double set_tx_gain(
        const double gain, const std::string& name_, const size_t chan) override
    {
        const std::string name   = name_.empty() ? "all" : name_;
        const fs_path gains_path = _get_frontend_path(TX_DIRECTION, chan) / "gains";
        return _tree->access<double>(gains_path / name / "value").set(gain).get();
    }

    double get_tx_gain(const std::string& name_, const size_t chan) override
    {
        const std::string name   = name_.empty() ? "all" : name_;
        const fs_path gains_path = _get_frontend_path(TX_DIRECTION, chan) / "gains";
        return _tree->access<double>(gains_path / name / "value").get();
    }

    std::vector<std::string> get_rx_gain_names(const size_t) const override
    {
        return {"all"};
    }

    gain_range_t get_rx_gain_range(
        const std::string& name, const size_t chan) const override
    {
        return _radio->get_rx_gain_range(name, chan);
    }

    // Like the tx variant of this method this will demonstrate the usage of the experts.
    double set_rx_gain(
        const double gain, const std::string& name_, const size_t chan) override
    {
        const std::string name   = name_.empty() ? "all" : name_;
        const fs_path gains_path = _get_frontend_path(RX_DIRECTION, chan) / "gains";
        return _tree->access<double>(gains_path / name / "value").set(gain).get();
    }

    double get_rx_gain(const std::string& name_, const size_t chan) override
    {
        const std::string name   = name_.empty() ? "all" : name_;
        const fs_path gains_path = _get_frontend_path(RX_DIRECTION, chan) / "gains";
        return _tree->access<double>(gains_path / name / "value").get();
    }

    // If methods are not available they may throw errors. Use with caution as this may
    // interrupt the functionality of an application.
    void set_rx_agc(const bool, const size_t) override
    {
        throw uhd::not_implemented_error("set_rx_agc is not supported on this radio");
    }

    meta_range_t get_tx_bandwidth_range(const size_t chan) const override
    {
        return _radio->get_tx_bandwidth_range(chan);
    }

    double get_tx_bandwidth(const size_t chan) override
    {
        return _radio->get_tx_bandwidth(chan);
    }

    double set_tx_bandwidth(const double bandwidth, const size_t chan) override
    {
        return _radio->set_tx_bandwidth(bandwidth, chan);
    }

    meta_range_t get_rx_bandwidth_range(const size_t chan) const override
    {
        return _radio->get_rx_bandwidth_range(chan);
    }

    double get_rx_bandwidth(const size_t chan) override
    {
        return _radio->get_rx_bandwidth(chan);
    }

    double set_rx_bandwidth(const double bandwidth, const size_t chan) override
    {
        return _radio->set_rx_bandwidth(bandwidth, chan);
    }

    // Example extension doesn't have LOs, so we return an empty vector.
    std::vector<std::string> get_rx_lo_names(const size_t) const override
    {
        return {};
    }

    std::vector<std::string> get_rx_lo_sources(
        const std::string&, const size_t) const override
    {
        return {};
    }

    freq_range_t get_rx_lo_freq_range(const std::string&, const size_t) const override
    {
        throw uhd::not_implemented_error(
            "get_rx_lo_freq_range is not supported on this radio");
    }

    void set_rx_lo_source(const std::string&, const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "set_rx_lo_source is not supported on this radio");
    }

    const std::string get_rx_lo_source(const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "get_rx_lo_source is not supported on this radio");
    }

    void set_rx_lo_export_enabled(bool, const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "set_rx_lo_export_enabled is not supported on this radio");
    }

    bool get_rx_lo_export_enabled(const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "get_rx_lo_export_enabled is not supported on this radio");
    }

    double set_rx_lo_freq(double, const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error("set_rx_lo_freq is not supported on this radio");
    }

    double get_rx_lo_freq(const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error("get_rx_lo_freq is not supported on this radio");
    }

    std::vector<std::string> get_tx_lo_names(const size_t) const override
    {
        return {};
    }

    std::vector<std::string> get_tx_lo_sources(
        const std::string&, const size_t) const override
    {
        return {};
    }

    freq_range_t get_tx_lo_freq_range(const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "get_tx_lo_freq_range is not supported on this radio");
    }

    void set_tx_lo_source(const std::string&, const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "set_tx_lo_source is not supported on this radio");
    }

    const std::string get_tx_lo_source(const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "get_tx_lo_source is not supported on this radio");
    }

    void set_tx_lo_export_enabled(const bool, const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "set_tx_lo_export_enabled is not supported on this radio");
    }

    bool get_tx_lo_export_enabled(const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error(
            "get_tx_lo_export_enabled is not supported on this radio");
    }

    double set_tx_lo_freq(const double, const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error("set_tx_lo_freq is not supported on this radio");
    }

    double get_tx_lo_freq(const std::string&, const size_t) override
    {
        throw uhd::not_implemented_error("get_tx_lo_freq is not supported on this radio");
    }

    // power_reference_iface
    bool has_rx_power_reference(const size_t chan) override
    {
        return _radio->has_rx_power_reference(chan);
    }

    void set_rx_power_reference(const double power_dbm, const size_t chan) override
    {
        _radio->set_rx_power_reference(power_dbm, chan);
    }

    double get_rx_power_reference(const size_t chan) override
    {
        return _radio->get_rx_power_reference(chan);
    }

    std::vector<std::string> get_rx_power_ref_keys(const size_t) override
    {
        throw uhd::not_implemented_error(
            "get_rx_power_ref_keys is not supported on this radio");
    }

    meta_range_t get_rx_power_range(const size_t) override
    {
        throw uhd::not_implemented_error(
            "get_rx_power_range is not supported on this radio");
    }

    bool has_tx_power_reference(const size_t chan) override
    {
        return _radio->has_tx_power_reference(chan);
    }

    void set_tx_power_reference(const double power_dbm, const size_t chan) override
    {
        _radio->set_tx_power_reference(power_dbm, chan);
    }

    double get_tx_power_reference(const size_t chan) override
    {
        return _radio->get_tx_power_reference(chan);
    }

    std::vector<std::string> get_tx_power_ref_keys(const size_t) override
    {
        throw uhd::not_implemented_error(
            "get_tx_power_ref_keys is not supported on this radio");
    }

    meta_range_t get_tx_power_range(const size_t) override
    {
        throw uhd::not_implemented_error(
            "get_tx_power_range is not supported on this radio");
    }

    std::string get_name() override
    {
        return "Example_Extension";
    }

private:
    uhd::rfnoc::radio_control::sptr _radio;

    uhd::experts::expert_container::sptr _expert_container;

    mutable uhd::property_tree::sptr _tree;

    // Necessary for writing into and reading from the property tree.
    fs_path _get_frontend_path(const direction_t dir, const size_t chan_idx) const
    {
        const std::string frontend = dir == TX_DIRECTION ? "tx_frontends"
                                                         : "rx_frontends";
        return frontend / chan_idx;
    }

    // Inits a path in the property tree to be able to use it later on.
    void init_path(const size_t chan, const uhd::direction_t trx);
};

} // namespace ext_example

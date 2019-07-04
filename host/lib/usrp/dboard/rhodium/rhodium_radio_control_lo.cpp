//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rhodium_constants.hpp"
#include "rhodium_radio_control.hpp"
#include <uhd/exception.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;

namespace {
constexpr int NUM_THRESHOLDS                                 = 13;
constexpr std::array<double, NUM_THRESHOLDS> FREQ_THRESHOLDS = {
    0.45e9, 0.5e9, 1e9, 1.5e9, 2e9, 2.5e9, 3e9, 3.55e9, 4e9, 4.5e9, 5e9, 5.5e9, 6e9};
constexpr std::array<int, NUM_THRESHOLDS> LMX_GAIN_VALUES = {
    18, 18, 17, 17, 17, 16, 12, 11, 11, 11, 10, 13, 18};
const std::array<int, NUM_THRESHOLDS> DSA_RX_GAIN_VALUES = {
    19, 19, 21, 21, 20, 20, 22, 21, 20, 22, 22, 24, 26};
const std::array<int, NUM_THRESHOLDS> DSA_TX_GAIN_VALUES = {
    19, 19, 21, 21, 20, 20, 22, 21, 22, 24, 24, 26, 28};

// Describes the lowband LO in terms of the master clock rate
const std::map<double, double> MCR_TO_LOWBAND_IF = {
    {200e6, 1200e6},
    {245.76e6, 1228.8e6},
    {250e6, 1500e6},
};

// validation helpers

std::vector<std::string> _get_lo_names()
{
    return {RHODIUM_LO1, RHODIUM_LO2};
}

void _validate_lo_name(const std::string& name, const std::string& function_name)
{
    if (!uhd::has(_get_lo_names(), name) and name != radio_control::ALL_LOS) {
        throw uhd::value_error(
            str(boost::format("%s was called with an invalid LO name: %s") % function_name
                % name));
    }
}

// object agnostic helpers
std::vector<std::string> _get_lo_sources(const std::string& name)
{
    if (name == RHODIUM_LO1 or name == radio_control::ALL_LOS) {
        return {"internal", "external"};
    } else {
        return {"internal"};
    }
}
} // namespace

/******************************************************************************
 * Property Getters
 *****************************************************************************/

std::vector<std::string> rhodium_radio_control_impl::get_tx_lo_names(
    const size_t chan) const
{
    RFNOC_LOG_TRACE("get_tx_lo_names(chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    return _get_lo_names();
}

std::vector<std::string> rhodium_radio_control_impl::get_rx_lo_names(
    const size_t chan) const
{
    RFNOC_LOG_TRACE("get_rx_lo_names(chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    return _get_lo_names();
}

std::vector<std::string> rhodium_radio_control_impl::get_tx_lo_sources(
    const std::string& name, const size_t chan) const
{
    RFNOC_LOG_TRACE("get_tx_lo_sources(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_tx_lo_sources");

    return _get_lo_sources(name);
}

std::vector<std::string> rhodium_radio_control_impl::get_rx_lo_sources(
    const std::string& name, const size_t chan) const
{
    RFNOC_LOG_TRACE("get_rx_lo_sources(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_rx_lo_sources");

    return _get_lo_sources(name);
}

freq_range_t rhodium_radio_control_impl::_get_lo_freq_range(const std::string& name) const
{
    if (name == RHODIUM_LO1) {
        return freq_range_t{RHODIUM_LO1_MIN_FREQ, RHODIUM_LO1_MAX_FREQ};
    } else if (name == RHODIUM_LO2) {
        // The Lowband LO is a fixed frequency
        return freq_range_t{_get_lowband_lo_freq(), _get_lowband_lo_freq()};
    } else {
        throw uhd::runtime_error(
            "LO frequency range must be retrieved for each stage individually");
    }
}

freq_range_t rhodium_radio_control_impl::get_tx_lo_freq_range(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_tx_lo_freq_range(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_tx_lo_freq_range");

    return _get_lo_freq_range(name);
}

freq_range_t rhodium_radio_control_impl::get_rx_lo_freq_range(
    const std::string& name, const size_t chan) const
{
    RFNOC_LOG_TRACE("get_rx_lo_freq_range(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_rx_lo_freq_range");

    return _get_lo_freq_range(name);
}

/******************************************************************************
 * Frequency Control
 *****************************************************************************/

double rhodium_radio_control_impl::set_tx_lo_freq(
    const double freq, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE(
        "set_tx_lo_freq(freq=" << freq << ", name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "set_tx_lo_freq");

    if (name == ALL_LOS) {
        throw uhd::runtime_error("LO frequency must be set for each stage individually");
    }
    if (name == RHODIUM_LO2) {
        RFNOC_LOG_WARNING("The Lowband LO cannot be tuned");
        return _get_lowband_lo_freq();
    }

    const auto sd_enabled   = _get_spur_dodging_enabled(TX_DIRECTION);
    const auto sd_threshold = _get_spur_dodging_threshold(TX_DIRECTION);

    _tx_lo_freq = _tx_lo->set_frequency(freq, sd_enabled, sd_threshold);
    set_tx_lo_gain(_get_lo_dsa_setting(_tx_lo_freq, TX_DIRECTION), RHODIUM_LO1, chan);
    set_tx_lo_power(_get_lo_power_setting(_tx_lo_freq), RHODIUM_LO1, chan);
    _cpld->set_tx_lo_path(_tx_lo_freq);

    return _tx_lo_freq;
}

double rhodium_radio_control_impl::set_rx_lo_freq(
    const double freq, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE(
        "set_rx_lo_freq(freq=" << freq << ", name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "set_rx_lo_freq");

    if (name == ALL_LOS) {
        throw uhd::runtime_error("LO frequency must be set for each stage individually");
    }
    if (name == RHODIUM_LO2) {
        RFNOC_LOG_WARNING("The Lowband LO cannot be tuned");
        return _get_lowband_lo_freq();
    }

    const auto sd_enabled   = _get_spur_dodging_enabled(RX_DIRECTION);
    const auto sd_threshold = _get_spur_dodging_threshold(RX_DIRECTION);

    _rx_lo_freq = _rx_lo->set_frequency(freq, sd_enabled, sd_threshold);
    set_rx_lo_gain(_get_lo_dsa_setting(_rx_lo_freq, RX_DIRECTION), RHODIUM_LO1, chan);
    set_rx_lo_power(_get_lo_power_setting(_rx_lo_freq), RHODIUM_LO1, chan);
    _cpld->set_rx_lo_path(_rx_lo_freq);

    return _rx_lo_freq;
}

double rhodium_radio_control_impl::get_tx_lo_freq(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_tx_lo_freq(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_tx_lo_freq");

    if (name == ALL_LOS) {
        throw uhd::runtime_error(
            "LO frequency must be retrieved for each stage individually");
    }

    return (name == RHODIUM_LO1) ? _tx_lo_freq : _get_lowband_lo_freq();
}

double rhodium_radio_control_impl::get_rx_lo_freq(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_rx_lo_freq(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_rx_lo_freq");

    if (name == ALL_LOS) {
        throw uhd::runtime_error(
            "LO frequency must be retrieved for each stage individually");
    }

    return (name == RHODIUM_LO1) ? _rx_lo_freq : _get_lowband_lo_freq();
}

/******************************************************************************
 * Source Control
 *****************************************************************************/

void rhodium_radio_control_impl::set_tx_lo_source(
    const std::string& src, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE(
        "set_tx_lo_source(src=" << src << ", name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "set_tx_lo_source");

    if (name == RHODIUM_LO2) {
        if (src != "internal") {
            throw uhd::value_error("The Lowband LO can only be set to internal");
        }
        return;
    }

    if (src == "internal") {
        _tx_lo->set_output_enable(lmx2592_iface::output_t::RF_OUTPUT_A, true);
        _cpld->set_tx_lo_source(
            rhodium_cpld_ctrl::tx_lo_input_sel_t::TX_LO_INPUT_SEL_INTERNAL);
    } else if (src == "external") {
        _tx_lo->set_output_enable(lmx2592_iface::output_t::RF_OUTPUT_A, false);
        _cpld->set_tx_lo_source(
            rhodium_cpld_ctrl::tx_lo_input_sel_t::TX_LO_INPUT_SEL_EXTERNAL);
    } else {
        throw uhd::value_error(
            str(boost::format("set_tx_lo_source was called with an invalid LO source: %s "
                              "Valid sources are [internal, external]")
                % src));
    }

    const bool enable_corrections = not _is_tx_lowband(get_tx_frequency(0))
                                    and src == "internal";
    _update_corrections(get_tx_frequency(0), TX_DIRECTION, enable_corrections);

    _tx_lo_source = src;
}

void rhodium_radio_control_impl::set_rx_lo_source(
    const std::string& src, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE(
        "set_rx_lo_source(src=" << src << ", name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "set_tx_lo_source");

    if (name == RHODIUM_LO2) {
        if (src != "internal") {
            throw uhd::value_error("The Lowband LO can only be set to internal");
        }
        return;
    }

    if (src == "internal") {
        _rx_lo->set_output_enable(lmx2592_iface::output_t::RF_OUTPUT_A, true);
        _cpld->set_rx_lo_source(
            rhodium_cpld_ctrl::rx_lo_input_sel_t::RX_LO_INPUT_SEL_INTERNAL);
    } else if (src == "external") {
        _rx_lo->set_output_enable(lmx2592_iface::output_t::RF_OUTPUT_A, false);
        _cpld->set_rx_lo_source(
            rhodium_cpld_ctrl::rx_lo_input_sel_t::RX_LO_INPUT_SEL_EXTERNAL);
    } else {
        throw uhd::value_error(
            str(boost::format("set_rx_lo_source was called with an invalid LO source: %s "
                              "Valid sources for LO1 are [internal, external]")
                % src));
    }

    const bool enable_corrections = not _is_rx_lowband(get_rx_frequency(0))
                                    and src == "internal";
    _update_corrections(get_rx_frequency(0), RX_DIRECTION, enable_corrections);

    _rx_lo_source = src;
}

const std::string rhodium_radio_control_impl::get_tx_lo_source(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_tx_lo_source(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_tx_lo_source");
    return (name == RHODIUM_LO1 or name == ALL_LOS) ? _tx_lo_source : "internal";
}

const std::string rhodium_radio_control_impl::get_rx_lo_source(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_rx_lo_source(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_rx_lo_source");
    return (name == RHODIUM_LO1 or name == ALL_LOS) ? _rx_lo_source : "internal";
}

/******************************************************************************
 * Export Control
 *****************************************************************************/

void rhodium_radio_control_impl::_set_lo1_export_enabled(
    const bool enabled, const direction_t dir)
{
    auto& _lo_ctrl = (dir == RX_DIRECTION) ? _rx_lo : _tx_lo;
    _lo_ctrl->set_output_enable(lmx2592_iface::output_t::RF_OUTPUT_B, enabled);
    if (_lo_dist_present) {
        const auto direction = (dir == RX_DIRECTION) ? "RX" : "TX";
        _rpcc->notify_with_token(_rpc_prefix + "enable_lo_export", direction, enabled);
    }
}

void rhodium_radio_control_impl::set_tx_lo_export_enabled(
    const bool enabled, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_tx_lo_export_enabled(enabled=" << enabled << ", name=" << name
                                                        << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "set_tx_lo_export_enabled");

    if (name == RHODIUM_LO2) {
        if (enabled) {
            throw uhd::value_error("The lowband LO cannot be exported");
        }
        return;
    }

    _set_lo1_export_enabled(enabled, TX_DIRECTION);
    _tx_lo_exported = enabled;
}

void rhodium_radio_control_impl::set_rx_lo_export_enabled(
    const bool enabled, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_rx_lo_export_enabled(enabled=" << enabled << ", name=" << name
                                                        << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "set_rx_lo_export_enabled");

    if (name == RHODIUM_LO2) {
        if (enabled) {
            throw uhd::value_error("The lowband LO cannot be exported");
        }
        return;
    }

    _set_lo1_export_enabled(enabled, RX_DIRECTION);
    _rx_lo_exported = enabled;
}

bool rhodium_radio_control_impl::get_tx_lo_export_enabled(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_tx_lo_export_enabled(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_tx_lo_export_enabled");

    return (name == RHODIUM_LO1 or name == ALL_LOS) ? _tx_lo_exported : false;
}

bool rhodium_radio_control_impl::get_rx_lo_export_enabled(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_rx_lo_export_enabled(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_rx_lo_export_enabled");

    return (name == RHODIUM_LO1 or name == ALL_LOS) ? _rx_lo_exported : false;
}

/******************************************************************************
 * LO Distribution Control
 *****************************************************************************/

void rhodium_radio_control_impl::_validate_output_port(
    const std::string& port_name, const std::string& function_name)
{
    if (!_lo_dist_present) {
        throw uhd::runtime_error(
            str(boost::format(
                    "%s can only be called if the LO distribution board was detected")
                % function_name));
    }

    if (!uhd::has(LO_OUTPUT_PORT_NAMES, port_name)) {
        throw uhd::value_error(
            str(boost::format("%s was called with an invalid LO output port: %s Valid "
                              "ports are [LO_OUT_0, LO_OUT_1, LO_OUT_2, LO_OUT_3]")
                % function_name % port_name));
    }
}

void rhodium_radio_control_impl::_set_lo_output_enabled(
    const bool enabled, const std::string& port_name, const direction_t dir)
{
    auto direction = (dir == RX_DIRECTION) ? "RX" : "TX";
    auto name_iter =
        std::find(LO_OUTPUT_PORT_NAMES.begin(), LO_OUTPUT_PORT_NAMES.end(), port_name);
    auto index = std::distance(LO_OUTPUT_PORT_NAMES.begin(), name_iter);

    _rpcc->notify_with_token(_rpc_prefix + "enable_lo_output", direction, index, enabled);
    auto out_enabled = (dir == RX_DIRECTION) ? _lo_dist_rx_out_enabled
                                             : _lo_dist_tx_out_enabled;
    out_enabled[index] = enabled;
}

void rhodium_radio_control_impl::set_tx_lo_output_enabled(
    const bool enabled, const std::string& port_name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_tx_lo_output_enabled(enabled=" << enabled
                                                        << ", port_name=" << port_name
                                                        << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_output_port(port_name, "set_tx_lo_output_enabled");

    _set_lo_output_enabled(enabled, port_name, TX_DIRECTION);
}

void rhodium_radio_control_impl::set_rx_lo_output_enabled(
    const bool enabled, const std::string& port_name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_rx_lo_output_enabled(enabled=" << enabled
                                                        << ", port_name=" << port_name
                                                        << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_output_port(port_name, "set_rx_lo_output_enabled");

    _set_lo_output_enabled(enabled, port_name, RX_DIRECTION);
}

bool rhodium_radio_control_impl::_get_lo_output_enabled(
    const std::string& port_name, const direction_t dir)
{
    auto name_iter =
        std::find(LO_OUTPUT_PORT_NAMES.begin(), LO_OUTPUT_PORT_NAMES.end(), port_name);
    auto index = std::distance(LO_OUTPUT_PORT_NAMES.begin(), name_iter);

    auto out_enabled = (dir == RX_DIRECTION) ? _lo_dist_rx_out_enabled
                                             : _lo_dist_tx_out_enabled;
    return out_enabled[index];
}

bool rhodium_radio_control_impl::get_tx_lo_output_enabled(
    const std::string& port_name, const size_t chan)
{
    RFNOC_LOG_TRACE(
        "get_tx_lo_output_enabled(port_name=" << port_name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_output_port(port_name, "get_tx_lo_output_enabled");

    return _get_lo_output_enabled(port_name, TX_DIRECTION);
}

bool rhodium_radio_control_impl::get_rx_lo_output_enabled(
    const std::string& port_name, const size_t chan)
{
    RFNOC_LOG_TRACE(
        "get_rx_lo_output_enabled(port_name=" << port_name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_output_port(port_name, "get_rx_lo_output_enabled");

    return _get_lo_output_enabled(port_name, RX_DIRECTION);
}

/******************************************************************************
 * Gain Control
 *****************************************************************************/

double rhodium_radio_control_impl::set_tx_lo_gain(
    double gain, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE(
        "set_tx_lo_gain(gain=" << gain << ", name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "set_tx_lo_gain");

    if (name == ALL_LOS) {
        throw uhd::runtime_error("LO gain must be set for each stage individually");
    }
    if (name == RHODIUM_LO2) {
        RFNOC_LOG_WARNING("The Lowband LO does not have configurable gain");
        return 0.0;
    }

    auto index = _get_lo_gain_range().clip(gain);

    _cpld->set_lo_gain(index, TX_DIRECTION);
    _lo_tx_gain = index;
    return _lo_tx_gain;
}

double rhodium_radio_control_impl::set_rx_lo_gain(
    double gain, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE(
        "set_rx_lo_gain(gain=" << gain << ", name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "set_rx_lo_gain");

    if (name == ALL_LOS) {
        throw uhd::runtime_error("LO gain must be set for each stage individually");
    }
    if (name == RHODIUM_LO2) {
        RFNOC_LOG_WARNING("The Lowband LO does not have configurable gain");
        return 0.0;
    }

    auto index = _get_lo_gain_range().clip(gain);

    _cpld->set_lo_gain(index, RX_DIRECTION);
    _lo_rx_gain = index;
    return _lo_rx_gain;
}

double rhodium_radio_control_impl::get_tx_lo_gain(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_tx_lo_gain(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_tx_lo_gain");

    if (name == ALL_LOS) {
        throw uhd::runtime_error("LO gain must be retrieved for each stage individually");
    }

    return (name == RHODIUM_LO1) ? _lo_rx_gain : 0.0;
}

double rhodium_radio_control_impl::get_rx_lo_gain(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_rx_lo_gain(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_rx_lo_gain");

    if (name == ALL_LOS) {
        throw uhd::runtime_error("LO gain must be retrieved for each stage individually");
    }

    return (name == RHODIUM_LO1) ? _lo_tx_gain : 0.0;
}

/******************************************************************************
 * Output Power Control
 *****************************************************************************/

double rhodium_radio_control_impl::_set_lo1_power(
    const double power, const direction_t dir)
{
    auto& _lo_ctrl     = (dir == RX_DIRECTION) ? _rx_lo : _tx_lo;
    auto coerced_power = static_cast<uint32_t>(_get_lo_power_range().clip(power, true));

    _lo_ctrl->set_output_power(lmx2592_iface::RF_OUTPUT_A, coerced_power);
    return coerced_power;
}

double rhodium_radio_control_impl::set_tx_lo_power(
    const double power, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_tx_lo_power(power=" << power << ", name=" << name
                                             << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "set_tx_lo_power");

    if (name == ALL_LOS) {
        throw uhd::runtime_error(
            "LO output power must be set for each stage individually");
    }
    if (name == RHODIUM_LO2) {
        RFNOC_LOG_WARNING("The Lowband LO does not have configurable output power");
        return 0.0;
    }

    _lo_tx_power = _set_lo1_power(power, TX_DIRECTION);
    return _lo_tx_power;
}

double rhodium_radio_control_impl::set_rx_lo_power(
    const double power, const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("set_rx_lo_power(power=" << power << ", name=" << name
                                             << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "set_rx_lo_power");

    if (name == ALL_LOS) {
        throw uhd::runtime_error(
            "LO output power must be set for each stage individually");
    }
    if (name == RHODIUM_LO2) {
        RFNOC_LOG_WARNING("The Lowband LO does not have configurable output power");
        return 0.0;
    }

    _lo_rx_power = _set_lo1_power(power, RX_DIRECTION);
    return _lo_rx_power;
}

double rhodium_radio_control_impl::get_tx_lo_power(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_tx_lo_power(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_tx_lo_power");

    if (name == ALL_LOS) {
        throw uhd::runtime_error(
            "LO output power must be retrieved for each stage individually");
    }

    return (name == RHODIUM_LO1) ? _lo_tx_power : 0.0;
}

double rhodium_radio_control_impl::get_rx_lo_power(
    const std::string& name, const size_t chan)
{
    RFNOC_LOG_TRACE("get_rx_lo_power(name=" << name << ", chan=" << chan << ")");
    UHD_ASSERT_THROW(chan == 0);
    _validate_lo_name(name, "get_rx_lo_power");

    if (name == ALL_LOS) {
        throw uhd::runtime_error(
            "LO output power must be retrieved for each stage individually");
    }

    return (name == RHODIUM_LO1) ? _lo_rx_power : 0.0;
}

/******************************************************************************
 * Helper Functions
 *****************************************************************************/

double rhodium_radio_control_impl::_get_lowband_lo_freq() const
{
    return MCR_TO_LOWBAND_IF.at(_master_clock_rate);
}

uhd::gain_range_t rhodium_radio_control_impl::_get_lo_gain_range()
{
    return gain_range_t(LO_MIN_GAIN, LO_MAX_GAIN, LO_GAIN_STEP);
}

uhd::gain_range_t rhodium_radio_control_impl::_get_lo_power_range()
{
    return gain_range_t(LO_MIN_POWER, LO_MAX_POWER, LO_POWER_STEP);
}

int rhodium_radio_control_impl::_get_lo_dsa_setting(
    const double freq, const direction_t dir)
{
    int index = 0;
    while (freq > FREQ_THRESHOLDS[index + 1]) {
        index++;
    }

    const double freq_low  = FREQ_THRESHOLDS[index];
    const double freq_high = FREQ_THRESHOLDS[index + 1];

    const auto& gain_table = (dir == RX_DIRECTION) ? DSA_RX_GAIN_VALUES
                                                   : DSA_TX_GAIN_VALUES;
    const double gain_low  = gain_table[index];
    const double gain_high = gain_table[index + 1];


    const double slope        = (gain_high - gain_low) / (freq_high - freq_low);
    const double gain_at_freq = gain_low + (freq - freq_low) * slope;

    RFNOC_LOG_TRACE("Interpolated DSA Gain is " << gain_at_freq);
    return static_cast<int>(std::round(gain_at_freq));
}

unsigned int rhodium_radio_control_impl::_get_lo_power_setting(double freq)
{
    int index = 0;
    while (freq > FREQ_THRESHOLDS[index + 1]) {
        index++;
    }

    const double freq_low      = FREQ_THRESHOLDS[index];
    const double freq_high     = FREQ_THRESHOLDS[index + 1];
    const double power_low     = LMX_GAIN_VALUES[index];
    const double power_high    = LMX_GAIN_VALUES[index + 1];
    const double slope         = (power_high - power_low) / (freq_high - freq_low);
    const double power_at_freq = power_low + (freq - freq_low) * slope;

    RFNOC_LOG_TRACE("Interpolated LMX Power is " << power_at_freq);
    return uhd::narrow_cast<unsigned int>(std::lround(power_at_freq));
}

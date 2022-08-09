//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "block_controller_factory_python.hpp"
#include <uhd/rfnoc/radio_control.hpp>

using namespace uhd::rfnoc;

void export_radio_control(py::module& m)
{
    // Re-import ALL_CHANS here to avoid linker errors
    const static auto ALL_CHANS = radio_control::ALL_CHANS;

    py::class_<radio_control, noc_block_base, radio_control::sptr>(m, "radio_control")
        .def(py::init(&block_controller_factory<radio_control>::make_from))
        .def("set_rate", &radio_control::set_rate)
        .def("get_rate", &radio_control::get_rate)
        .def("get_rate_range", &radio_control::get_rate_range)
        .def("get_spc", &radio_control::get_spc)
        .def("get_tx_antenna", &radio_control::get_tx_antenna)
        .def("get_tx_antennas", &radio_control::get_tx_antennas)
        .def("set_tx_antenna", &radio_control::set_tx_antenna)
        .def("get_rx_antenna", &radio_control::get_rx_antenna)
        .def("get_rx_antennas", &radio_control::get_rx_antennas)
        .def("set_rx_antenna", &radio_control::set_rx_antenna)
        .def("get_tx_frequency", &radio_control::get_tx_frequency)
        .def("set_tx_frequency", &radio_control::set_tx_frequency)
        .def("set_tx_tune_args", &radio_control::set_tx_tune_args)
        .def("get_tx_frequency_range", &radio_control::get_tx_frequency_range)
        .def("get_rx_frequency", &radio_control::get_rx_frequency)
        .def("set_rx_frequency", &radio_control::set_rx_frequency)
        .def("set_rx_tune_args", &radio_control::set_rx_tune_args)
        .def("get_rx_frequency_range", &radio_control::get_rx_frequency_range)
        .def("get_tx_gain_names", &radio_control::get_tx_gain_names)
        .def("get_tx_gain_range",
            py::overload_cast<const size_t>(
                &radio_control::get_tx_gain_range, py::const_),
            py::arg("chan"))
        .def("get_tx_gain_range",
            py::overload_cast<const std::string&, const size_t>(
                &radio_control::get_tx_gain_range, py::const_),
            py::arg("name"),
            py::arg("chan"))
        .def("get_tx_gain",
            py::overload_cast<const size_t>(&radio_control::get_tx_gain),
            py::arg("chan"))
        .def("get_tx_gain",
            py::overload_cast<const std::string&, const size_t>(
                &radio_control::get_tx_gain),
            py::arg("name"),
            py::arg("chan"))
        .def("set_tx_gain",
            py::overload_cast<const double, const size_t>(&radio_control::set_tx_gain),
            py::arg("gain"),
            py::arg("chan"))
        .def("set_tx_gain",
            py::overload_cast<const double, const std::string&, const size_t>(
                &radio_control::set_tx_gain),
            py::arg("gain"),
            py::arg("name"),
            py::arg("chan"))
        .def("has_tx_power_reference", &radio_control::has_tx_power_reference)
        .def("set_tx_power_reference", &radio_control::set_tx_power_reference)
        .def("get_tx_power_reference", &radio_control::get_tx_power_reference)
        .def("get_tx_power_ref_keys",
            &radio_control::get_tx_power_ref_keys,
            py::arg("chan") = 0)
        .def("get_rx_gain_names", &radio_control::get_rx_gain_names)
        .def("get_rx_gain_range",
            py::overload_cast<const size_t>(
                &radio_control::get_rx_gain_range, py::const_),
            py::arg("chan"))
        .def("get_rx_gain_range",
            py::overload_cast<const std::string&, const size_t>(
                &radio_control::get_rx_gain_range, py::const_),
            py::arg("name"),
            py::arg("chan"))
        .def("get_rx_gain",
            py::overload_cast<const size_t>(&radio_control::get_rx_gain),
            py::arg("chan"))
        .def("get_rx_gain",
            py::overload_cast<const std::string&, const size_t>(
                &radio_control::get_rx_gain),
            py::arg("name"),
            py::arg("chan"))
        .def("set_rx_gain",
            py::overload_cast<const double, const size_t>(&radio_control::set_rx_gain),
            py::arg("gain"),
            py::arg("chan"))
        .def("set_rx_gain",
            py::overload_cast<const double, const std::string&, const size_t>(
                &radio_control::set_rx_gain),
            py::arg("gain"),
            py::arg("name"),
            py::arg("chan"))
        .def("set_rx_agc", &radio_control::set_rx_agc)
        .def("has_rx_power_reference", &radio_control::has_rx_power_reference)
        .def("set_rx_power_reference", &radio_control::set_rx_power_reference)
        .def("get_rx_power_reference", &radio_control::get_rx_power_reference)
        .def("get_rx_power_ref_keys",
            &radio_control::get_rx_power_ref_keys,
            py::arg("chan") = 0)
        .def("get_tx_gain_profile_names", &radio_control::get_tx_gain_profile_names)
        .def("get_rx_gain_profile_names", &radio_control::get_rx_gain_profile_names)
        .def("set_tx_gain_profile", &radio_control::set_tx_gain_profile)
        .def("set_rx_gain_profile", &radio_control::set_rx_gain_profile)
        .def("get_tx_gain_profile", &radio_control::get_tx_gain_profile)
        .def("get_rx_gain_profile", &radio_control::get_rx_gain_profile)
        .def("get_tx_bandwidth_range", &radio_control::get_tx_bandwidth_range)
        .def("get_tx_bandwidth", &radio_control::get_tx_bandwidth)
        .def("set_tx_bandwidth", &radio_control::set_tx_bandwidth)
        .def("get_rx_bandwidth_range", &radio_control::get_rx_bandwidth_range)
        .def("get_rx_bandwidth", &radio_control::get_rx_bandwidth)
        .def("set_rx_bandwidth", &radio_control::set_rx_bandwidth)
        .def("get_rx_lo_names", &radio_control::get_rx_lo_names)
        .def("get_rx_lo_sources", &radio_control::get_rx_lo_sources)
        .def("get_rx_lo_freq_range", &radio_control::get_rx_lo_freq_range)
        .def("set_rx_lo_source", &radio_control::set_rx_lo_source)
        .def("get_rx_lo_source", &radio_control::get_rx_lo_source)
        .def("set_rx_lo_export_enabled", &radio_control::set_rx_lo_export_enabled)
        .def("get_rx_lo_export_enabled", &radio_control::get_rx_lo_export_enabled)
        .def("set_rx_lo_freq", &radio_control::set_rx_lo_freq)
        .def("get_rx_lo_freq", &radio_control::get_rx_lo_freq)
        .def("get_tx_lo_names", &radio_control::get_tx_lo_names)
        .def("get_tx_lo_sources", &radio_control::get_tx_lo_sources)
        .def("get_tx_lo_freq_range", &radio_control::get_tx_lo_freq_range)
        .def("set_tx_lo_source", &radio_control::set_tx_lo_source)
        .def("get_tx_lo_source", &radio_control::get_tx_lo_source)
        .def("set_tx_lo_export_enabled", &radio_control::set_tx_lo_export_enabled)
        .def("get_tx_lo_export_enabled", &radio_control::get_tx_lo_export_enabled)
        .def("set_tx_lo_freq", &radio_control::set_tx_lo_freq)
        .def("get_tx_lo_freq", &radio_control::get_tx_lo_freq)
        .def("set_tx_dc_offset", &radio_control::set_tx_dc_offset)
        .def("get_tx_dc_offset_range", &radio_control::get_tx_dc_offset_range)
        .def("set_tx_iq_balance", &radio_control::set_tx_iq_balance)
        .def("set_rx_dc_offset",
            py::overload_cast<const bool, size_t>(&radio_control::set_rx_dc_offset),
            py::arg("enb"),
            py::arg("chan") = ALL_CHANS)
        .def("set_rx_dc_offset",
            py::overload_cast<const std::complex<double>&, size_t>(
                &radio_control::set_rx_dc_offset),
            py::arg("offset"),
            py::arg("chan"))
        .def("get_rx_dc_offset_range", &radio_control::get_rx_dc_offset_range)
        .def("set_rx_iq_balance",
            py::overload_cast<const bool, size_t>(&radio_control::set_rx_iq_balance),
            py::arg("enb"),
            py::arg("chan"))
        .def("set_rx_iq_balance",
            py::overload_cast<const std::complex<double>&, size_t>(
                &radio_control::set_rx_iq_balance),
            py::arg("correction"),
            py::arg("chan"))
        .def("get_gpio_banks", &radio_control::get_gpio_banks)
        .def("set_gpio_attr", &radio_control::set_gpio_attr)
        .def("get_gpio_attr", &radio_control::get_gpio_attr)
        .def("get_rx_sensor_names", &radio_control::get_rx_sensor_names)
        .def("get_rx_sensor", &radio_control::get_rx_sensor)
        .def("get_tx_sensor_names", &radio_control::get_tx_sensor_names)
        .def("get_tx_sensor", &radio_control::get_tx_sensor)
        .def("issue_stream_cmd", &radio_control::issue_stream_cmd)
        .def("enable_rx_timestamps", &radio_control::enable_rx_timestamps)
        .def("get_slot_name", &radio_control::get_slot_name)
        .def("get_chan_from_dboard_fe", &radio_control::get_chan_from_dboard_fe)
        .def("get_dboard_fe_from_chan", &radio_control::get_dboard_fe_from_chan)
        .def("get_fe_name", &radio_control::get_fe_name)
        .def("set_db_eeprom", &radio_control::set_db_eeprom)
        .def("get_db_eeprom", &radio_control::get_db_eeprom);
}

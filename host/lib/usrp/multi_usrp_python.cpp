//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019-2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include "multi_usrp_python.hpp"
#include <uhd/usrp/multi_usrp.hpp>

void export_multi_usrp(py::module& m)
{
    using multi_usrp = uhd::usrp::multi_usrp;

    const auto ALL_MBOARDS = multi_usrp::ALL_MBOARDS;
    const auto ALL_CHANS   = multi_usrp::ALL_CHANS;
    const auto ALL_LOS     = multi_usrp::ALL_LOS;

    // clang-format off
    py::class_<multi_usrp, multi_usrp::sptr>(m, "multi_usrp")

        // Factory
        .def(py::init(&multi_usrp::make))

        // clang-format off
        .def("get_tree"                , [](multi_usrp& self){ return self.get_tree().get(); }, py::return_value_policy::reference_internal)

        // General USRP methods
        .def("get_rx_freq"             , &multi_usrp::get_rx_freq, py::arg("chan") = 0)
        .def("get_rx_num_channels"     , &multi_usrp::get_rx_num_channels)
        .def("get_rx_rate"             , &multi_usrp::get_rx_rate, py::arg("chan") = 0)
        .def("get_rx_stream"           , &multi_usrp::get_rx_stream)
        .def("set_rx_freq"             , &multi_usrp::set_rx_freq, py::arg("tune_request"), py::arg("chan") = 0)
        .def("set_rx_gain"             , (void (multi_usrp::*)(double, const std::string&, size_t)) &multi_usrp::set_rx_gain, py::arg("gain"), py::arg("name"), py::arg("chan") = 0)
        .def("set_rx_gain"             , (void (multi_usrp::*)(double, size_t)) &multi_usrp::set_rx_gain, py::arg("gain"), py::arg("chan") = 0)
        .def("set_rx_rate"             , &multi_usrp::set_rx_rate, py::arg("rate"), py::arg("chan") = ALL_CHANS)
        .def("set_rx_spp"              , &multi_usrp::set_rx_spp, py::arg("spp"), py::arg("chan") = ALL_CHANS)
        .def("get_tx_freq"             , &multi_usrp::get_tx_freq, py::arg("chan") = 0)
        .def("get_tx_num_channels"     , &multi_usrp::get_tx_num_channels)
        .def("get_tx_rate"             , &multi_usrp::get_tx_rate, py::arg("chan") = 0)
        .def("get_tx_stream"           , &multi_usrp::get_tx_stream)
        .def("set_tx_freq"             , &multi_usrp::set_tx_freq, py::arg("tune_request"), py::arg("chan") = 0)
        .def("set_tx_gain"             , (void (multi_usrp::*)(double, const std::string&, size_t)) &multi_usrp::set_tx_gain, py::arg("gain"), py::arg("name"), py::arg("chan") = 0)
        .def("set_tx_gain"             , (void (multi_usrp::*)(double, size_t)) &multi_usrp::set_tx_gain, py::arg("gain"), py::arg("chan") = 0)
        .def("set_tx_rate"             , &multi_usrp::set_tx_rate, py::arg("rate"), py::arg("chan") = ALL_CHANS)
        .def("get_usrp_rx_info",
            [](multi_usrp& self, const size_t chan = 0) {
                return static_cast<std::map<std::string, std::string>>(
                    self.get_usrp_rx_info(chan));
            },
            py::arg("chan") = 0)
        .def("get_usrp_tx_info",
            [](multi_usrp& self, const size_t chan = 0) {
                return static_cast<std::map<std::string, std::string>>(
                    self.get_usrp_tx_info(chan));
            },
            py::arg("chan") = 0)
        .def("set_master_clock_rate"   , &multi_usrp::set_master_clock_rate, py::arg("rate"), py::arg("mboard") = ALL_MBOARDS)
        .def("get_master_clock_rate"   , &multi_usrp::get_master_clock_rate, py::arg("mboard") = 0)
        .def("get_master_clock_rate_range", &multi_usrp::get_master_clock_rate_range, py::arg("mboard") = ALL_MBOARDS)
        .def("get_pp_string"           , &multi_usrp::get_pp_string)
        .def("get_mboard_name"         , &multi_usrp::get_mboard_name, py::arg("mboard") = 0)
        .def("get_time_now"            , &multi_usrp::get_time_now, py::arg("mboard") = 0)
        .def("get_time_last_pps"       , &multi_usrp::get_time_last_pps, py::arg("mboard") = 0)
        .def("set_time_now"            , &multi_usrp::set_time_now, py::arg("time_spec"), py::arg("mboard") = ALL_MBOARDS)
        .def("set_time_next_pps"       , &multi_usrp::set_time_next_pps, py::arg("time_spec"), py::arg("mboard") = ALL_MBOARDS)
        .def("set_time_unknown_pps"    , &multi_usrp::set_time_unknown_pps)
        .def("get_time_synchronized"   , &multi_usrp::get_time_synchronized)
        .def("set_command_time"        , &multi_usrp::set_command_time, py::arg("time_spec"), py::arg("mboard") = ALL_MBOARDS)
        .def("clear_command_time"      , &multi_usrp::clear_command_time, py::arg("mboard") = ALL_MBOARDS)
        .def("issue_stream_cmd"        , &multi_usrp::issue_stream_cmd, py::arg("rate"), py::arg("chan") = ALL_CHANS)
        .def("set_time_source"         , &multi_usrp::set_time_source, py::arg("source"), py::arg("mboard") = ALL_MBOARDS)
        .def("get_time_source"         , &multi_usrp::get_time_source)
        .def("get_time_sources"        , &multi_usrp::get_time_sources)
        .def("set_clock_source"        , &multi_usrp::set_clock_source, py::arg("source"), py::arg("mboard") = ALL_MBOARDS)
        .def("get_clock_source"        , &multi_usrp::get_clock_source)
        .def("get_clock_sources"       , &multi_usrp::get_clock_sources)
        .def("set_sync_source"         , (void (multi_usrp::*)(const std::string&, const std::string&, size_t)) &multi_usrp::set_sync_source, py::arg("clock_source"), py::arg("time_source"), py::arg("mboard") = ALL_MBOARDS)
        .def("set_sync_source"         , (void (multi_usrp::*)(const uhd::device_addr_t&, size_t)) &multi_usrp::set_sync_source, py::arg("sync_source"), py::arg("mboard") = ALL_MBOARDS)
        .def("get_sync_source"         , &multi_usrp::get_sync_source)
        .def("get_sync_sources"        , &multi_usrp::get_sync_sources)
        .def("set_clock_source_out"    , &multi_usrp::set_clock_source_out, py::arg("enb"), py::arg("mboard") = ALL_MBOARDS)
        .def("set_time_source_out"     , &multi_usrp::set_time_source_out, py::arg("enb"), py::arg("mboard") = ALL_MBOARDS)
        .def("get_num_mboards"         , &multi_usrp::get_num_mboards)
        .def("get_mboard_sensor"       , &multi_usrp::get_mboard_sensor, py::arg("name"), py::arg("mboard") = 0)
        .def("get_mboard_sensor_names" , &multi_usrp::get_mboard_sensor_names, py::arg("mboard") = 0)
        .def("set_user_register"       , &multi_usrp::set_user_register, py::arg("addr"), py::arg("data"), py::arg("mboard") = ALL_MBOARDS)
        .def("get_radio_control"       , [](multi_usrp& self, const size_t chan){ return &self.get_radio_control(chan); }, py::arg("chan") = 0, py::return_value_policy::reference_internal)
        .def("get_mb_controller"       , [](multi_usrp& self, const size_t chan){ return &self.get_mb_controller(chan); }, py::arg("mboard") = 0, py::return_value_policy::reference_internal)

        // RX methods
        .def("set_rx_subdev_spec"      , &multi_usrp::set_rx_subdev_spec, py::arg("spec"), py::arg("mboard") = ALL_MBOARDS)
        .def("get_rx_subdev_spec"      , &multi_usrp::get_rx_subdev_spec, py::arg("mboard") = 0)
        .def("get_rx_subdev_name"      , &multi_usrp::get_rx_subdev_name, py::arg("chan") = 0)
        .def("get_rx_rates"            , &multi_usrp::get_rx_rates, py::arg("chan") = 0)
        .def("get_rx_freq_range"       , &multi_usrp::get_rx_freq_range, py::arg("chan") = 0)
        .def("get_fe_rx_freq_range"    , &multi_usrp::get_fe_rx_freq_range, py::arg("chan") = 0)
        .def("get_rx_lo_names"         , &multi_usrp::get_rx_lo_names, py::arg("chan") = 0)
        .def("set_rx_lo_source"        , &multi_usrp::set_rx_lo_source, py::arg("src"), py::arg("name") = ALL_LOS, py::arg("chan") = 0)
        .def("get_rx_lo_source"        , &multi_usrp::get_rx_lo_source, py::arg("name") = ALL_LOS, py::arg("chan") = 0)
        .def("get_rx_lo_sources"       , &multi_usrp::get_rx_lo_sources, py::arg("name") = ALL_LOS, py::arg("chan") = 0)
        .def("set_rx_lo_export_enabled", &multi_usrp::set_rx_lo_export_enabled, py::arg("enb"), py::arg("name") = ALL_LOS, py::arg("chan") = 0)
        .def("get_rx_lo_export_enabled", &multi_usrp::get_rx_lo_export_enabled, py::arg("name") = ALL_LOS, py::arg("chan") = 0)
        .def("set_rx_lo_freq"          , &multi_usrp::set_rx_lo_freq, py::arg("freq"), py::arg("name"), py::arg("chan") = 0)
        .def("get_rx_lo_freq"          , &multi_usrp::get_rx_lo_freq, py::arg("name"), py::arg("chan") = 0)
        .def("get_rx_lo_freq_range"    , &multi_usrp::get_rx_lo_freq_range, py::arg("name"), py::arg("chan") = 0)
        .def("set_normalized_rx_gain"  , &multi_usrp::set_normalized_rx_gain, py::arg("gain"), py::arg("chan") = 0)
        .def("get_normalized_rx_gain"  , &multi_usrp::get_normalized_rx_gain, py::arg("chan") = 0)
        .def("set_rx_agc"              , &multi_usrp::set_rx_agc, py::arg("enable"), py::arg("chan") = 0)
        .def("get_rx_gain"             , (double (multi_usrp::*)(const std::string&, size_t)) &multi_usrp::get_rx_gain, py::arg("name"), py::arg("chan") = 0)
        .def("get_rx_gain"             , (double (multi_usrp::*)(size_t)) &multi_usrp::get_rx_gain, py::arg("chan") = 0)
        .def("get_rx_gain_range"       , (uhd::gain_range_t (multi_usrp::*)(const std::string&, size_t)) &multi_usrp::get_rx_gain_range, py::arg("name"), py::arg("chan") = 0)
        .def("get_rx_gain_range"       , (uhd::gain_range_t (multi_usrp::*)(size_t)) &multi_usrp::get_rx_gain_range, py::arg("chan") = 0)
        .def("get_rx_gain_names"       , &multi_usrp::get_rx_gain_names, py::arg("chan") = 0)
        .def("set_rx_antenna"          , &multi_usrp::set_rx_antenna, py::arg("ant"), py::arg("chan") = 0)
        .def("get_rx_antenna"          , &multi_usrp::get_rx_antenna, py::arg("chan") = 0)
        .def("get_rx_antennas"         , &multi_usrp::get_rx_antennas, py::arg("chan") = 0)
        .def("set_rx_bandwidth"        , &multi_usrp::set_rx_bandwidth, py::arg("bandwidth"), py::arg("chan") = 0)
        .def("get_rx_bandwidth"        , &multi_usrp::get_rx_bandwidth, py::arg("chan") = 0)
        .def("get_rx_bandwidth_range"  , &multi_usrp::get_rx_bandwidth_range, py::arg("chan") = 0)
        .def("get_rx_dboard_iface"     , &multi_usrp::get_rx_dboard_iface, py::arg("chan") = 0)
        .def("get_rx_sensor"           , &multi_usrp::get_rx_sensor, py::arg("name"), py::arg("chan") = 0)
        .def("get_rx_sensor_names"     , &multi_usrp::get_rx_sensor_names, py::arg("chan") = 0)
        .def("set_rx_dc_offset"        , (void (multi_usrp::*)(const std::complex<double>&, size_t)) &multi_usrp::set_rx_dc_offset, py::arg("offset"), py::arg("chan") = 0)
        .def("set_rx_dc_offset"        , (void (multi_usrp::*)(bool, size_t)) &multi_usrp::set_rx_dc_offset, py::arg("enb"), py::arg("chan") = 0)
        .def("set_rx_iq_balance"       , (void (multi_usrp::*)(const std::complex<double>&, size_t)) &multi_usrp::set_rx_iq_balance, py::arg("correction"), py::arg("chan") = 0)
        .def("set_rx_iq_balance"       , (void (multi_usrp::*)(bool, size_t)) &multi_usrp::set_rx_iq_balance, py::arg("enb"), py::arg("chan") = 0)
        .def("get_rx_gain_profile"     , &multi_usrp::get_rx_gain_profile, py::arg("chan") = 0)
        .def("set_rx_gain_profile"     , &multi_usrp::set_rx_gain_profile, py::arg("profile"), py::arg("chan") = 0)
        .def("get_rx_gain_profile_names", &multi_usrp::get_rx_gain_profile_names, py::arg("chan") = 0)
        .def("has_rx_power_reference"  , &multi_usrp::has_rx_power_reference, py::arg("chan") = 0)
        .def("set_rx_power_reference"  , &multi_usrp::set_rx_power_reference, py::arg("power_dbm"), py::arg("chan") = 0)
        .def("get_rx_power_reference"  , &multi_usrp::get_rx_power_reference, py::arg("chan") = 0)
        .def("get_rx_power_range"      , &multi_usrp::get_rx_power_range, py::arg("chan") = 0)

        // TX methods
        .def("set_tx_subdev_spec"      , &multi_usrp::set_tx_subdev_spec, py::arg("spec"), py::arg("mboard") = ALL_MBOARDS)
        .def("get_tx_subdev_spec"      , &multi_usrp::get_tx_subdev_spec, py::arg("mboard") = 0)
        .def("get_tx_subdev_name"      , &multi_usrp::get_tx_subdev_name, py::arg("chan") = 0)
        .def("get_tx_rates"            , &multi_usrp::get_tx_rates, py::arg("chan") = 0)
        .def("get_tx_freq_range"       , &multi_usrp::get_tx_freq_range, py::arg("chan") = 0)
        .def("get_fe_tx_freq_range"    , &multi_usrp::get_fe_tx_freq_range, py::arg("chan") = 0)
        .def("get_tx_lo_names"         , &multi_usrp::get_tx_lo_names, py::arg("chan") = 0)
        .def("set_tx_lo_source"        , &multi_usrp::set_tx_lo_source, py::arg("src"), py::arg("name") = ALL_LOS, py::arg("chan") = 0)
        .def("get_tx_lo_source"        , &multi_usrp::get_tx_lo_source, py::arg("name") = ALL_LOS, py::arg("chan") = 0)
        .def("get_tx_lo_sources"       , &multi_usrp::get_tx_lo_sources, py::arg("name") = ALL_LOS, py::arg("chan") = 0)
        .def("set_tx_lo_export_enabled", &multi_usrp::set_tx_lo_export_enabled, py::arg("enb"), py::arg("name") = ALL_LOS, py::arg("chan") = 0)
        .def("get_tx_lo_export_enabled", &multi_usrp::get_tx_lo_export_enabled, py::arg("name") = ALL_LOS, py::arg("chan") = 0)
        .def("set_tx_lo_freq"          , &multi_usrp::set_tx_lo_freq, py::arg("freq"), py::arg("name"), py::arg("chan") = 0)
        .def("get_tx_lo_freq"          , &multi_usrp::get_tx_lo_freq, py::arg("name"), py::arg("chan") = 0)
        .def("get_tx_lo_freq_range"    , &multi_usrp::get_tx_lo_freq_range, py::arg("name"), py::arg("chan") = 0)
        .def("set_normalized_tx_gain"  , &multi_usrp::set_normalized_tx_gain, py::arg("gain"), py::arg("chan") = 0)
        .def("get_normalized_tx_gain"  , &multi_usrp::get_normalized_tx_gain, py::arg("chan") = 0)
        .def("get_tx_gain"             , (double (multi_usrp::*)(const std::string&, size_t)) &multi_usrp::get_tx_gain, py::arg("name"), py::arg("chan") = 0)
        .def("get_tx_gain"             , (double (multi_usrp::*)(size_t)) &multi_usrp::get_tx_gain, py::arg("chan") = 0)
        .def("get_tx_gain_range"       , (uhd::gain_range_t (multi_usrp::*)(const std::string&, size_t)) &multi_usrp::get_tx_gain_range, py::arg("name"), py::arg("chan") = 0)
        .def("get_tx_gain_range"       , (uhd::gain_range_t (multi_usrp::*)(size_t)) &multi_usrp::get_tx_gain_range, py::arg("chan") = 0)
        .def("get_tx_gain_names"       , &multi_usrp::get_tx_gain_names, py::arg("chan") = 0)
        .def("set_tx_antenna"          , &multi_usrp::set_tx_antenna, py::arg("ant"), py::arg("chan") = 0)
        .def("get_tx_antenna"          , &multi_usrp::get_tx_antenna, py::arg("chan") = 0)
        .def("get_tx_antennas"         , &multi_usrp::get_tx_antennas, py::arg("chan") = 0)
        .def("set_tx_bandwidth"        , &multi_usrp::set_tx_bandwidth, py::arg("bandwidth"), py::arg("chan") = 0)
        .def("get_tx_bandwidth"        , &multi_usrp::get_tx_bandwidth, py::arg("chan") = 0)
        .def("get_tx_bandwidth_range"  , &multi_usrp::get_tx_bandwidth_range, py::arg("chan") = 0)
        .def("get_tx_dboard_iface"     , &multi_usrp::get_tx_dboard_iface, py::arg("chan") = 0)
        .def("get_tx_sensor"           , &multi_usrp::get_tx_sensor, py::arg("name"), py::arg("chan") = 0)
        .def("get_tx_sensor_names"     , &multi_usrp::get_tx_sensor_names, py::arg("chan") = 0)
        .def("set_tx_dc_offset"        , (void (multi_usrp::*)(const std::complex<double>&, size_t)) &multi_usrp::set_tx_dc_offset, py::arg("offset"), py::arg("chan") = 0)
        .def("set_tx_iq_balance"       , (void (multi_usrp::*)(const std::complex<double>&, size_t)) &multi_usrp::set_tx_iq_balance, py::arg("correction"), py::arg("chan") = 0)
        .def("get_tx_gain_profile"     , &multi_usrp::get_tx_gain_profile, py::arg("chan") = 0)
        .def("set_tx_gain_profile"     , &multi_usrp::set_tx_gain_profile, py::arg("profile"), py::arg("chan") = 0)
        .def("get_tx_gain_profile_names", &multi_usrp::get_tx_gain_profile_names, py::arg("chan") = 0)
        .def("has_tx_power_reference"  , &multi_usrp::has_tx_power_reference, py::arg("chan") = 0)
        .def("set_tx_power_reference"  , &multi_usrp::set_tx_power_reference, py::arg("power_dbm"), py::arg("chan") = 0)
        .def("get_tx_power_reference"  , &multi_usrp::get_tx_power_reference, py::arg("chan") = 0)
        .def("get_tx_power_range"      , &multi_usrp::get_tx_power_range, py::arg("chan") = 0)

        // GPIO methods
        .def("get_gpio_banks"          , &multi_usrp::get_gpio_banks)
        .def("set_gpio_attr"           , (void (multi_usrp::*)(const std::string&, const std::string&, const uint32_t, const uint32_t, const size_t)) &multi_usrp::set_gpio_attr, py::arg("bank"), py::arg("attr"), py::arg("value"), py::arg("mask") = 0xffffffff, py::arg("mboard") = 0)
        .def("get_gpio_attr"           , &multi_usrp::get_gpio_attr, py::arg("bank"), py::arg("attr"), py::arg("mboard") = 0)
        .def("get_gpio_srcs"           , &multi_usrp::get_gpio_srcs, py::arg("bank"), py::arg("mboard") = 0)
        .def("get_gpio_src"            , &multi_usrp::get_gpio_src, py::arg("bank"), py::arg("mboard") = 0)
        .def("set_gpio_src"            , &multi_usrp::set_gpio_src, py::arg("bank"), py::arg("src"), py::arg("mboard") = 0)
        .def("get_gpio_src_banks"      , &multi_usrp::get_gpio_src_banks, py::arg("mboard") = 0)

        // Filter API methods
        .def("get_rx_filter_names"     , &multi_usrp::get_rx_filter_names)
        .def("get_rx_filter"           , &multi_usrp::get_rx_filter)
        .def("set_rx_filter"           , &multi_usrp::set_rx_filter)
        .def("get_tx_filter_names"     , &multi_usrp::get_tx_filter_names)
        .def("get_tx_filter"           , &multi_usrp::get_tx_filter)
        .def("set_tx_filter"           , &multi_usrp::set_tx_filter)
        // clang-format off
        ;
    // clang-format on
}

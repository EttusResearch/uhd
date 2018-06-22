//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_MULTI_USRP_PYTHON_HPP
#define INCLUDED_UHD_USRP_MULTI_USRP_PYTHON_HPP

#include <uhd/usrp/multi_usrp.hpp>

//
// Boost.Python needs overloaded API calls to be defined
//
static void set_rx_gain_0(
    uhd::usrp::multi_usrp *multi_usrp, double gain, const std::string &name, size_t chan = 0)
{
    multi_usrp->set_rx_gain(gain, name, chan);
}

static void set_rx_gain_1(
    uhd::usrp::multi_usrp *multi_usrp, double gain, size_t chan = 0)
{
    multi_usrp->set_rx_gain(gain, chan);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_rx_gain_0, set_rx_gain_0, 3, 4);
BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_rx_gain_1, set_rx_gain_1, 2, 3);

static void set_tx_gain_0(
    uhd::usrp::multi_usrp *multi_usrp, double gain, const std::string &name, size_t chan = 0)
{
    multi_usrp->set_tx_gain(gain, name, chan);
}

static void set_tx_gain_1(
    uhd::usrp::multi_usrp *multi_usrp, double gain, size_t chan = 0)
{
    multi_usrp->set_tx_gain(gain, chan);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_tx_gain_0, set_tx_gain_0, 3, 4);
BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_tx_gain_1, set_tx_gain_1, 2, 3);

static double get_rx_gain_0(
    uhd::usrp::multi_usrp *multi_usrp, const std::string &name, size_t chan = 0)
{
    return multi_usrp->get_rx_gain(name, chan);
}

static double get_rx_gain_1(
    uhd::usrp::multi_usrp *multi_usrp, size_t chan = 0)
{
    return multi_usrp->get_rx_gain(chan);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(overload_get_rx_gain_0, get_rx_gain_0, 2, 3);
BOOST_PYTHON_FUNCTION_OVERLOADS(overload_get_rx_gain_1, get_rx_gain_1, 1, 2);

static double get_tx_gain_0(
    uhd::usrp::multi_usrp *multi_usrp, const std::string &name, size_t chan = 0)
{
    return multi_usrp->get_tx_gain(name, chan);
}

static double get_tx_gain_1(
    uhd::usrp::multi_usrp *multi_usrp, size_t chan = 0)
{
    return multi_usrp->get_tx_gain(chan);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(overload_get_tx_gain_0, get_tx_gain_0, 2, 3);
BOOST_PYTHON_FUNCTION_OVERLOADS(overload_get_tx_gain_1, get_tx_gain_1, 1, 2);

static uhd::gain_range_t get_rx_gain_range_0(
    uhd::usrp::multi_usrp *multi_usrp, const std::string &name, size_t chan = 0)
{
    return multi_usrp->get_rx_gain_range(name, chan);
}

static uhd::gain_range_t get_rx_gain_range_1(
    uhd::usrp::multi_usrp *multi_usrp, size_t chan = 0)
{
    return multi_usrp->get_rx_gain_range(chan);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(overload_get_rx_gain_range_0, get_rx_gain_range_0, 2, 3);
BOOST_PYTHON_FUNCTION_OVERLOADS(overload_get_rx_gain_range_1, get_rx_gain_range_1, 1, 2);

static uhd::gain_range_t get_tx_gain_range_0(
    uhd::usrp::multi_usrp *multi_usrp, const std::string &name, size_t chan = 0)
{
    return multi_usrp->get_tx_gain_range(name, chan);
}

static uhd::gain_range_t get_tx_gain_range_1(
    uhd::usrp::multi_usrp *multi_usrp, size_t chan = 0)
{
    return multi_usrp->get_tx_gain_range(chan);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(overload_get_tx_gain_range_0, get_tx_gain_range_0, 2, 3);
BOOST_PYTHON_FUNCTION_OVERLOADS(overload_get_tx_gain_range_1, get_tx_gain_range_1, 1, 2);

static void set_rx_dc_offset_0(
    uhd::usrp::multi_usrp *multi_usrp, const bool enb, size_t chan = 0)
{
    multi_usrp->set_rx_dc_offset(enb, chan);
}

static void set_rx_dc_offset_1(
    uhd::usrp::multi_usrp *multi_usrp, const std::complex<double> &offset, size_t chan = 0)
{
    multi_usrp->set_rx_dc_offset(offset, chan);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_rx_dc_offset_0, set_rx_dc_offset_0, 2, 3);
BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_rx_dc_offset_1, set_rx_dc_offset_1, 2, 3);

static void set_tx_dc_offset_0(
    uhd::usrp::multi_usrp *multi_usrp, const bool enb, size_t chan = 0)
{
    multi_usrp->set_tx_dc_offset(enb, chan);
}

static void set_tx_dc_offset_1(
    uhd::usrp::multi_usrp *multi_usrp, const std::complex<double> &offset, size_t chan = 0)
{
    multi_usrp->set_tx_dc_offset(offset, chan);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_tx_dc_offset_0, set_tx_dc_offset_0, 2, 3);
BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_tx_dc_offset_1, set_tx_dc_offset_1, 2, 3);

static void set_rx_iq_balance_0(
    uhd::usrp::multi_usrp *multi_usrp, const bool enb, size_t chan = 0)
{
    multi_usrp->set_rx_iq_balance(enb, chan);
}

static void set_rx_iq_balance_1(
    uhd::usrp::multi_usrp *multi_usrp, const std::complex<double> &offset, size_t chan = 0)
{
    multi_usrp->set_rx_iq_balance(offset, chan);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_rx_iq_balance_0, set_rx_iq_balance_0, 2, 3);
BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_rx_iq_balance_1, set_rx_iq_balance_1, 2, 3);

static void set_gpio_attr_0(uhd::usrp::multi_usrp *multi_usrp,
                            const std::string &bank,
                            const std::string &attr,
                            const uint32_t value,
                            const uint32_t mask = 0xffffffff,
                            const size_t mboard = 0)
{
    multi_usrp->set_gpio_attr(bank, attr, value, mask, mboard);
}

static void set_gpio_attr_1(uhd::usrp::multi_usrp *multi_usrp,
                            const std::string &bank,
                            const std::string &attr,
                            const std::string &value,
                            const uint32_t mask = 0xffffffff,
                            const size_t mboard = 0)
{
    multi_usrp->set_gpio_attr(bank, attr, value, mask, mboard);
}

BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_gpio_attr_0, set_gpio_attr_0, 4, 6);
BOOST_PYTHON_FUNCTION_OVERLOADS(overload_set_gpio_attr_1, set_gpio_attr_1, 4, 6);

//
// Boost.Python needs to know about default argument overloads
//

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_freq, get_rx_freq, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_rate, get_rx_rate, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_rx_freq, set_rx_freq, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_rx_rate, set_rx_rate, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_freq, get_tx_freq, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_rate, get_tx_rate, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_tx_freq, set_tx_freq, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_tx_rate, set_tx_rate, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_usrp_rx_info, get_usrp_rx_info, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_usrp_tx_info, get_usrp_tx_info, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_master_clock_rate, set_master_clock_rate, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_master_clock_rate, get_master_clock_rate, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_master_clock_rate_range, get_master_clock_rate_range, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_mboard_name, get_mboard_name, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_time_now, get_time_now, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_time_last_pps, get_time_last_pps, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_time_now, set_time_now, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_time_next_pps, set_time_next_pps, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_command_time, set_command_time, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_clear_command_time, clear_command_time, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_issue_stream_cmd, issue_stream_cmd, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_clock_config, set_clock_config, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_time_source, set_time_source, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_clock_source, set_clock_source, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_clock_source_out, set_clock_source_out, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_time_source_out, set_time_source_out, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_mboard_sensor, get_mboard_sensor, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_mboard_sensor_names, get_mboard_sensor_names, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_user_register, set_user_register, 2, 3);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_rx_subdev_spec, set_rx_subdev_spec, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_subdev_spec, get_rx_subdev_spec, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_subdev_name, get_rx_subdev_name, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_rates, get_rx_rates, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_freq_range, get_rx_freq_range, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_fe_rx_freq_range, get_fe_rx_freq_range, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_lo_names, get_rx_lo_names, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_lo_source, get_rx_lo_source, 0, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_rx_lo_source, set_rx_lo_source, 1, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_lo_sources, get_rx_lo_sources, 0, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_rx_lo_export_enabled, set_rx_lo_export_enabled, 1, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_lo_export_enabled, get_rx_lo_export_enabled, 0, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_rx_lo_freq, set_rx_lo_freq, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_lo_freq, get_rx_lo_freq, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_lo_freq_range, get_rx_lo_freq_range, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_normalized_rx_gain, set_normalized_rx_gain, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_normalized_rx_gain, get_normalized_rx_gain, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_rx_agc, set_rx_agc, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_gain_names, get_rx_gain_names, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_rx_antenna, set_rx_antenna, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_antenna, get_rx_antenna, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_antennas, get_rx_antennas, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_rx_bandwidth, set_rx_bandwidth, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_bandwidth, get_rx_bandwidth, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_bandwidth_range, get_rx_bandwidth_range, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_dboard_iface, get_rx_dboard_iface, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_sensor, get_rx_sensor, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_sensor_names, get_rx_sensor_names, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_gain_profile, get_rx_gain_profile, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_rx_gain_profile, set_rx_gain_profile, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_rx_gain_profile_names, get_rx_gain_profile_names, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_tx_subdev_spec, set_tx_subdev_spec, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_subdev_spec, get_tx_subdev_spec, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_subdev_name, get_tx_subdev_name, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_rates, get_tx_rates, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_freq_range, get_tx_freq_range, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_fe_tx_freq_range, get_fe_tx_freq_range, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_lo_names, get_tx_lo_names, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_lo_source, get_tx_lo_source, 0, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_tx_lo_source, set_tx_lo_source, 1, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_lo_sources, get_tx_lo_sources, 0, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_tx_lo_export_enabled, set_tx_lo_export_enabled, 1, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_lo_export_enabled, get_tx_lo_export_enabled, 0, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_tx_lo_freq, set_tx_lo_freq, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_lo_freq, get_tx_lo_freq, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_lo_freq_range, get_tx_lo_freq_range, 1, 2);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_normalized_tx_gain, set_normalized_tx_gain, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_normalized_tx_gain, get_normalized_tx_gain, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_gain_names, get_tx_gain_names, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_tx_antenna, set_tx_antenna, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_antenna, get_tx_antenna, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_antennas, get_tx_antennas, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_tx_bandwidth, set_tx_bandwidth, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_bandwidth, get_tx_bandwidth, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_bandwidth_range, get_tx_bandwidth_range, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_dboard_iface, get_tx_dboard_iface, 0, 1);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_sensor, get_tx_sensor, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_sensor_names, get_tx_sensor_names, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_tx_iq_balance, set_tx_iq_balance, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_gain_profile, get_tx_gain_profile, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_set_tx_gain_profile, set_tx_gain_profile, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_tx_gain_profile_names, get_tx_gain_profile_names, 0, 1);


BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_gpio_attr, get_gpio_attr, 2, 3);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_enumerate_registers, enumerate_registers, 0, 1);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_register_info, get_register_info, 1, 2);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_write_register, write_register, 3, 4);
BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_read_register, read_register, 2, 3);

BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(overload_get_filter_names, get_filter_names, 0, 1);

void export_multi_usrp()
{
    using multi_usrp      = uhd::usrp::multi_usrp;
    using register_info_t = multi_usrp::register_info_t;

    bp::class_<register_info_t>("register_info")
        .add_property("bitwidth", &register_info_t::bitwidth)
        .add_property("readable", &register_info_t::readable)
        .add_property("writable", &register_info_t::writable)
        ;

    bp::class_<
        multi_usrp,
        boost::shared_ptr<multi_usrp>,
        boost::noncopyable>("multi_usrp", bp::no_init)

        .def("__init__", bp::make_constructor(&multi_usrp::make))

        // Methods
        .def("make", &multi_usrp::make)
        .staticmethod("make")

        // General USRP methods
        .def("get_rx_freq"             , &multi_usrp::get_rx_freq, overload_get_rx_freq())
        .def("get_rx_num_channels"     , &multi_usrp::get_rx_num_channels)
        .def("get_rx_rate"             , &multi_usrp::get_rx_rate, overload_get_rx_rate())
        .def("get_rx_stream"           , &multi_usrp::get_rx_stream) 
        .def("set_rx_freq"             , &multi_usrp::set_rx_freq, overload_set_rx_freq())
        .def("set_rx_gain"             , &set_rx_gain_0, overload_set_rx_gain_0())
        .def("set_rx_gain"             , &set_rx_gain_1, overload_set_rx_gain_1())
        .def("set_rx_rate"             , &multi_usrp::set_rx_rate, overload_set_rx_rate())
        .def("get_tx_freq"             , &multi_usrp::get_tx_freq, overload_get_tx_freq())
        .def("get_tx_num_channels"     , &multi_usrp::get_tx_num_channels)
        .def("get_tx_rate"             , &multi_usrp::get_tx_rate, overload_get_tx_rate())
        .def("get_tx_stream"           , &multi_usrp::get_tx_stream)
        .def("set_tx_freq"             , &multi_usrp::set_tx_freq, overload_set_tx_freq())
        .def("set_tx_gain"             , &set_tx_gain_0, overload_set_tx_gain_0())
        .def("set_tx_gain"             , &set_tx_gain_1, overload_set_tx_gain_1())
        .def("set_tx_rate"             , &multi_usrp::set_tx_rate, overload_set_tx_rate())
        .def("get_usrp_rx_info"        , &multi_usrp::get_usrp_rx_info, overload_get_usrp_rx_info())
        .def("get_usrp_tx_info"        , &multi_usrp::get_usrp_tx_info, overload_get_usrp_tx_info())
        .def("set_master_clock_rate"   , &multi_usrp::set_master_clock_rate, overload_set_master_clock_rate())
        .def("get_master_clock_rate"   , &multi_usrp::get_master_clock_rate, overload_get_master_clock_rate())
        .def("get_master_clock_rate_range", &multi_usrp::get_master_clock_rate_range, overload_get_master_clock_rate_range())
        .def("get_pp_string"           , &multi_usrp::get_pp_string)
        .def("get_mboard_name"         , &multi_usrp::get_mboard_name, overload_get_mboard_name())
        .def("get_time_now"            , &multi_usrp::get_time_now, overload_get_time_now())
        .def("get_time_last_pps"       , &multi_usrp::get_time_last_pps, overload_get_time_last_pps())
        .def("set_time_now"            , &multi_usrp::set_time_now, overload_set_time_now())
        .def("set_time_next_pps"       , &multi_usrp::set_time_next_pps, overload_set_time_next_pps())
        .def("set_time_unknown_pps"    , &multi_usrp::set_time_unknown_pps)
        .def("get_time_synchronized"   , &multi_usrp::get_time_synchronized)
        .def("set_command_time"        , &multi_usrp::set_command_time, overload_set_command_time())
        .def("clear_command_time"      , &multi_usrp::clear_command_time, overload_clear_command_time())
        .def("issue_stream_cmd"        , &multi_usrp::issue_stream_cmd, overload_issue_stream_cmd())
        .def("set_clock_config"        , &multi_usrp::set_clock_config, overload_set_clock_config())
        .def("set_time_source"         , &multi_usrp::set_time_source, overload_set_time_source())
        .def("get_time_source"         , &multi_usrp::get_time_source)
        .def("get_time_sources"        , &multi_usrp::get_time_sources)
        .def("set_clock_source"        , &multi_usrp::set_clock_source, overload_set_clock_source())
        .def("get_clock_source"        , &multi_usrp::get_clock_source)
        .def("get_clock_sources"       , &multi_usrp::get_clock_sources)
        .def("set_clock_source_out"    , &multi_usrp::set_clock_source_out, overload_set_clock_source_out())
        .def("set_time_source_out"     , &multi_usrp::set_time_source_out, overload_set_time_source_out())
        .def("get_num_mboards"         , &multi_usrp::get_num_mboards)
        .def("get_mboard_sensor"       , &multi_usrp::get_mboard_sensor, overload_get_mboard_sensor())
        .def("get_mboard_sensor_names" , &multi_usrp::get_mboard_sensor_names, overload_get_mboard_sensor_names())
        .def("set_user_register"       , &multi_usrp::set_user_register, overload_set_user_register())

        // RX methods
        .def("set_rx_subdev_spec"      , &multi_usrp::set_rx_subdev_spec, overload_set_rx_subdev_spec())
        .def("get_rx_subdev_spec"      , &multi_usrp::get_rx_subdev_spec, overload_get_rx_subdev_spec())
        .def("get_rx_subdev_name"      , &multi_usrp::get_rx_subdev_name, overload_get_rx_subdev_name())
        .def("get_rx_rates"            , &multi_usrp::get_rx_rates, overload_get_rx_rates())
        .def("get_rx_freq_range"       , &multi_usrp::get_rx_freq_range, overload_get_rx_freq_range())
        .def("get_fe_rx_freq_range"    , &multi_usrp::get_fe_rx_freq_range, overload_get_fe_rx_freq_range())
        .def("get_rx_lo_names"         , &multi_usrp::get_rx_lo_names, overload_get_rx_lo_names())
        .def("set_rx_lo_source"        , &multi_usrp::set_rx_lo_source, overload_set_rx_lo_source())
        .def("get_rx_lo_source"        , &multi_usrp::get_rx_lo_source, overload_get_rx_lo_source())
        .def("get_rx_lo_sources"       , &multi_usrp::get_rx_lo_sources, overload_get_rx_lo_sources())
        .def("set_rx_lo_export_enabled", &multi_usrp::set_rx_lo_export_enabled, overload_set_rx_lo_export_enabled())
        .def("get_rx_lo_export_enabled", &multi_usrp::get_rx_lo_export_enabled, overload_get_rx_lo_export_enabled())
        .def("set_rx_lo_freq"          , &multi_usrp::set_rx_lo_freq, overload_set_rx_lo_freq())
        .def("get_rx_lo_freq"          , &multi_usrp::get_rx_lo_freq, overload_get_rx_lo_freq())
        .def("get_rx_lo_freq_range"    , &multi_usrp::get_rx_lo_freq_range, overload_get_rx_lo_freq_range())
        .def("set_normalized_rx_gain"  , &multi_usrp::set_normalized_rx_gain, overload_set_normalized_rx_gain())
        .def("get_normalized_rx_gain"  , &multi_usrp::get_normalized_rx_gain, overload_get_normalized_rx_gain())
        .def("set_rx_agc"              , &multi_usrp::set_rx_agc, overload_set_rx_agc())
        .def("get_rx_gain"             , &get_rx_gain_0, overload_get_rx_gain_0())
        .def("get_rx_gain"             , &get_rx_gain_1, overload_get_rx_gain_1())
        .def("get_rx_gain_range"       , &get_rx_gain_range_0, overload_get_rx_gain_range_0())
        .def("get_rx_gain_range"       , &get_rx_gain_range_1, overload_get_rx_gain_range_1())
        .def("get_rx_gain_names"       , &multi_usrp::get_rx_gain_names, overload_get_rx_gain_names())
        .def("set_rx_antenna"          , &multi_usrp::set_rx_antenna, overload_set_rx_antenna())
        .def("get_rx_antenna"          , &multi_usrp::get_rx_antenna, overload_get_rx_antenna())
        .def("get_rx_antennas"         , &multi_usrp::get_rx_antennas, overload_get_rx_antennas())
        .def("set_rx_bandwidth"        , &multi_usrp::set_rx_bandwidth, overload_set_rx_bandwidth())
        .def("get_rx_bandwidth"        , &multi_usrp::get_rx_bandwidth, overload_get_rx_bandwidth())
        .def("get_rx_bandwidth_range"  , &multi_usrp::get_rx_bandwidth_range, overload_get_rx_bandwidth_range())
        .def("get_rx_dboard_iface"     , &multi_usrp::get_rx_dboard_iface, overload_get_rx_dboard_iface())
        .def("get_rx_sensor"           , &multi_usrp::get_rx_sensor, overload_get_rx_sensor())
        .def("get_rx_sensor_names"     , &multi_usrp::get_rx_sensor_names, overload_get_rx_sensor_names())
        .def("set_rx_dc_offset"        , &set_rx_dc_offset_0, overload_set_rx_dc_offset_0())
        .def("set_rx_dc_offset"        , &set_rx_dc_offset_1, overload_set_rx_dc_offset_1())
        .def("set_rx_iq_balance"       , &set_rx_iq_balance_0, overload_set_rx_iq_balance_0())
        .def("set_rx_iq_balance"       , &set_rx_iq_balance_1, overload_set_rx_iq_balance_1())
        .def("get_rx_gain_profile"     , &multi_usrp::get_rx_gain_profile, overload_get_rx_gain_profile())
        .def("set_rx_gain_profile"     , &multi_usrp::set_rx_gain_profile, overload_set_rx_gain_profile())
        .def("get_rx_gain_profile_names", &multi_usrp::get_rx_gain_profile_names, overload_get_rx_gain_profile_names())

        // TX methods
        .def("set_tx_subdev_spec"      , &multi_usrp::set_tx_subdev_spec, overload_set_tx_subdev_spec())
        .def("get_tx_subdev_spec"      , &multi_usrp::get_tx_subdev_spec, overload_get_tx_subdev_spec())
        .def("get_tx_subdev_name"      , &multi_usrp::get_tx_subdev_name, overload_get_tx_subdev_name())
        .def("get_tx_rates"            , &multi_usrp::get_tx_rates, overload_get_rx_rates())
        .def("get_tx_freq_range"       , &multi_usrp::get_tx_freq_range, overload_get_tx_freq_range())
        .def("get_fe_tx_freq_range"    , &multi_usrp::get_fe_tx_freq_range, overload_get_fe_tx_freq_range())
        .def("get_tx_lo_names"         , &multi_usrp::get_tx_lo_names, overload_get_tx_lo_names())
        .def("set_tx_lo_source"        , &multi_usrp::set_tx_lo_source, overload_set_tx_lo_source())
        .def("get_tx_lo_source"        , &multi_usrp::get_tx_lo_source, overload_get_tx_lo_source())
        .def("get_tx_lo_sources"       , &multi_usrp::get_tx_lo_sources, overload_get_tx_lo_sources())
        .def("set_tx_lo_export_enabled", &multi_usrp::set_tx_lo_export_enabled, overload_set_tx_lo_export_enabled())
        .def("get_tx_lo_export_enabled", &multi_usrp::get_tx_lo_export_enabled, overload_get_tx_lo_export_enabled())
        .def("set_tx_lo_freq"          , &multi_usrp::set_tx_lo_freq, overload_set_tx_lo_freq())
        .def("get_tx_lo_freq"          , &multi_usrp::get_tx_lo_freq, overload_get_tx_lo_freq())
        .def("get_tx_lo_freq_range"    , &multi_usrp::get_tx_lo_freq_range, overload_get_tx_lo_freq_range())
        .def("set_normalized_tx_gain"  , &multi_usrp::set_normalized_tx_gain, overload_set_normalized_tx_gain())
        .def("get_normalized_tx_gain"  , &multi_usrp::get_normalized_tx_gain, overload_get_normalized_tx_gain())
        .def("get_tx_gain"             , &get_tx_gain_0, overload_get_tx_gain_0())
        .def("get_tx_gain"             , &get_tx_gain_1, overload_get_tx_gain_1())
        .def("get_tx_gain_range"       , &get_tx_gain_range_0, overload_get_tx_gain_range_0())
        .def("get_tx_gain_range"       , &get_tx_gain_range_1, overload_get_tx_gain_range_1())
        .def("get_tx_gain_names"       , &multi_usrp::get_tx_gain_names, overload_get_tx_gain_names())
        .def("set_tx_antenna"          , &multi_usrp::set_tx_antenna, overload_set_tx_antenna())
        .def("get_tx_antenna"          , &multi_usrp::get_tx_antenna, overload_get_tx_antenna())
        .def("get_tx_antennas"         , &multi_usrp::get_tx_antennas, overload_get_tx_antennas())
        .def("set_tx_bandwidth"        , &multi_usrp::set_tx_bandwidth, overload_set_tx_bandwidth())
        .def("get_tx_bandwidth"        , &multi_usrp::get_tx_bandwidth, overload_get_tx_bandwidth())
        .def("get_tx_bandwidth_range"  , &multi_usrp::get_tx_bandwidth_range, overload_get_tx_bandwidth_range())
        .def("get_tx_dboard_iface"     , &multi_usrp::get_tx_dboard_iface, overload_get_tx_dboard_iface())
        .def("get_tx_sensor"           , &multi_usrp::get_tx_sensor, overload_get_tx_sensor())
        .def("get_tx_sensor_names"     , &multi_usrp::get_tx_sensor_names, overload_get_tx_sensor_names())
        .def("set_tx_dc_offset"        , &set_tx_dc_offset_0, overload_set_tx_dc_offset_0())
        .def("set_tx_dc_offset"        , &set_tx_dc_offset_1, overload_set_tx_dc_offset_1())
        .def("set_tx_iq_balance"       , &multi_usrp::set_tx_iq_balance, overload_set_tx_iq_balance())
        .def("get_tx_gain_profile"     , &multi_usrp::get_tx_gain_profile, overload_get_tx_gain_profile())
        .def("set_tx_gain_profile"     , &multi_usrp::set_tx_gain_profile, overload_set_tx_gain_profile())
        .def("get_tx_gain_profile_names", &multi_usrp::get_tx_gain_profile_names, overload_get_tx_gain_profile_names())

        // GPIO methods
        .def("get_gpio_banks"          , &multi_usrp::get_gpio_banks)
        .def("set_gpio_attr"           , &set_gpio_attr_0, overload_set_gpio_attr_0())
        .def("set_gpio_attr"           , &set_gpio_attr_1, overload_set_gpio_attr_1())
        .def("get_gpio_attr"           , &multi_usrp::get_gpio_attr, overload_get_gpio_attr())
        .def("enumerate_registers"     , &multi_usrp::enumerate_registers, overload_enumerate_registers())
        .def("get_register_info"       , &multi_usrp::get_register_info, overload_get_register_info())
        .def("write_register"          , &multi_usrp::write_register, overload_write_register())
        .def("read_register"           , &multi_usrp::read_register, overload_read_register())

        // Filter API methods
        .def("get_filter_names"        , &multi_usrp::get_filter_names, overload_get_filter_names())
        .def("get_filter"              , &multi_usrp::get_filter)
        .def("set_filter"              , &multi_usrp::set_filter)
        ;
}

#endif /* INCLUDED_UHD_USRP_MULTI_USRP_PYTHON_HPP */

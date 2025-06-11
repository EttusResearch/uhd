//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <include/uhdlib/usrp/common/max287x.hpp>
#include <pybind11/functional.h>

void export_max287x_iface(py::module& m)
{
    using output_power_t        = max287x_iface::output_power_t;
    using aux_output_power_t    = max287x_iface::aux_output_power_t;
    using muxout_mode_t         = max287x_iface::muxout_mode_t;
    using charge_pump_current_t = max287x_iface::charge_pump_current_t;
    using ld_pin_mode_t         = max287x_iface::ld_pin_mode_t;
    using rf_output_port_t      = max287x_iface::rf_output_port_t;

    py::enum_<output_power_t>(m, "output_power")
        .value("OUTPUT_POWER_M4DBM", output_power_t::OUTPUT_POWER_M4DBM)
        .value("OUTPUT_POWER_M1DBM", output_power_t::OUTPUT_POWER_M1DBM)
        .value("OUTPUT_POWER_2DBM", output_power_t::OUTPUT_POWER_2DBM)
        .value("OUTPUT_POWER_5DBM", output_power_t::OUTPUT_POWER_5DBM);

    py::enum_<aux_output_power_t>(m, "aux_output_power")
        .value("AUX_OUTPUT_POWER_M4DBM", aux_output_power_t::AUX_OUTPUT_POWER_M4DBM)
        .value("AUX_OUTPUT_POWER_M1DBM", aux_output_power_t::AUX_OUTPUT_POWER_M1DBM)
        .value("AUX_OUTPUT_POWER_2DBM", aux_output_power_t::AUX_OUTPUT_POWER_2DBM)
        .value("AUX_OUTPUT_POWER_5DBM", aux_output_power_t::AUX_OUTPUT_POWER_5DBM);

    py::enum_<muxout_mode_t>(m, "muxout_mode")
        .value("MUXOUT_TRI_STATE", muxout_mode_t::MUXOUT_TRI_STATE)
        .value("MUXOUT_HIGH", muxout_mode_t::MUXOUT_HIGH)
        .value("MUXOUT_LOW", muxout_mode_t::MUXOUT_LOW)
        .value("MUXOUT_RDIV", muxout_mode_t::MUXOUT_RDIV)
        .value("MUXOUT_NDIV", muxout_mode_t::MUXOUT_NDIV)
        .value("MUXOUT_ALD", muxout_mode_t::MUXOUT_ALD)
        .value("MUXOUT_DLD", muxout_mode_t::MUXOUT_DLD)
        .value("MUXOUT_SYNC", muxout_mode_t::MUXOUT_SYNC)
        .value("MUXOUT_SPI", muxout_mode_t::MUXOUT_SPI);

    py::enum_<charge_pump_current_t>(m, "charge_pump_current")
        .value("CHARGE_PUMP_CURRENT_0_32MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_0_32MA)
        .value("CHARGE_PUMP_CURRENT_0_64MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_0_64MA)
        .value("CHARGE_PUMP_CURRENT_0_96MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_0_96MA)
        .value("CHARGE_PUMP_CURRENT_1_28MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_1_28MA)
        .value("CHARGE_PUMP_CURRENT_1_60MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_1_60MA)
        .value("CHARGE_PUMP_CURRENT_1_92MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_1_92MA)
        .value("CHARGE_PUMP_CURRENT_2_24MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_2_24MA)
        .value("CHARGE_PUMP_CURRENT_2_56MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_2_56MA)
        .value("CHARGE_PUMP_CURRENT_2_88MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_2_88MA)
        .value("CHARGE_PUMP_CURRENT_3_20MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_3_20MA)
        .value("CHARGE_PUMP_CURRENT_3_52MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_3_52MA)
        .value("CHARGE_PUMP_CURRENT_3_84MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_3_84MA)
        .value("CHARGE_PUMP_CURRENT_4_16MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_4_16MA)
        .value("CHARGE_PUMP_CURRENT_4_48MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_4_48MA)
        .value("CHARGE_PUMP_CURRENT_4_80MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_4_80MA)
        .value("CHARGE_PUMP_CURRENT_5_12MA",
            charge_pump_current_t::CHARGE_PUMP_CURRENT_5_12MA);

    py::enum_<ld_pin_mode_t>(m, "ld_pin_mode")
        .value("LD_PIN_MODE_LOW", ld_pin_mode_t::LD_PIN_MODE_LOW)
        .value("LD_PIN_MODE_DLD", ld_pin_mode_t::LD_PIN_MODE_DLD)
        .value("LD_PIN_MODE_ALD", ld_pin_mode_t::LD_PIN_MODE_ALD)
        .value("LD_PIN_MODE_HIGH", ld_pin_mode_t::LD_PIN_MODE_HIGH);

    py::enum_<rf_output_port_t>(m, "rf_output_port")
        .value("RF_OUT", rf_output_port_t::RF_OUT)
        .value("AUX_OUT", rf_output_port_t::AUX_OUT)
        .value("BOTH", rf_output_port_t::BOTH);

    py::class_<max287x_iface, max287x_iface::sptr>(m, "max_2871")
        .def(py::init(&max287x_iface::make<max2871>))
        // Methods
        .def("set_frequency",
            &max287x_iface::set_frequency,
            py::arg("target_freq"),
            py::arg("ref_freq"),
            py::arg("target_pfd_freq"),
            py::arg("is_int_n"),
            py::arg("output_port") = rf_output_port_t::RF_OUT)
        .def("power_up", &max287x_iface::power_up)
        .def("shutdown", &max287x_iface::shutdown)
        .def("is_shutdown", &max287x_iface::is_shutdown)
        .def("set_output_power", &max287x_iface::set_output_power)
        .def("set_aux_output_power", &max287x_iface::set_aux_output_power)
        .def("set_aux_output_power_enable", &max287x_iface::set_aux_output_power_enable)
        .def("commit", &max287x_iface::commit)
        .def("can_sync", &max287x_iface::can_sync)
        .def("config_for_sync", &max287x_iface::config_for_sync)
        .def("set_auto_retune", &max287x_iface::set_auto_retune)
        .def("set_charge_pump_current", &max287x_iface::set_charge_pump_current)
        .def("set_muxout_mode", &max287x_iface::set_muxout_mode)
        .def("set_ld_pin_mode", &max287x_iface::set_ld_pin_mode)
        .def("get_register", &max287x_iface::get_register)
        .def("set_register",
            &max287x_iface::set_register,
            py::arg("addr"),
            py::arg("mask"),
            py::arg("value"),
            py::arg("commit") = true);
}

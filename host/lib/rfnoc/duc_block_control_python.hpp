//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/duc_block_control.hpp>

using namespace uhd::rfnoc;

void export_duc_block_control(py::module& m)
{
    py::class_<duc_block_control, noc_block_base, duc_block_control::sptr>(
        m, "duc_block_control")
        .def(py::init(&block_controller_factory<duc_block_control>::make_from))
        .def("set_freq",
            &duc_block_control::set_freq,
            py::arg("freq"),
            py::arg("chan"),
            py::arg("time") = boost::optional<uhd::time_spec_t>())
        .def("get_freq", &duc_block_control::get_freq)
        .def("get_frequency_range", &duc_block_control::get_frequency_range)
        .def("get_input_rate", &duc_block_control::get_input_rate)
        .def("get_output_rate", &duc_block_control::get_output_rate)
        .def("set_output_rate", &duc_block_control::set_output_rate)
        .def("get_input_rates", &duc_block_control::get_input_rates)
        .def("set_input_rate", &duc_block_control::set_input_rate);
}

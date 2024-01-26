//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/window_block_control.hpp>

using namespace uhd::rfnoc;

void export_window_block_control(py::module& m)
{
    py::class_<window_block_control, noc_block_base, window_block_control::sptr>(
        m, "window_block_control")
        .def(py::init(&block_controller_factory<window_block_control>::make_from))
        .def("get_max_num_coefficients", &window_block_control::get_max_num_coefficients)
        .def("set_coefficients", &window_block_control::set_coefficients)
        .def("get_coefficients", &window_block_control::get_coefficients);
}

//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/fir_filter_block_control.hpp>

using namespace uhd::rfnoc;

void export_fir_filter_block_control(py::module& m)
{
    py::class_<fir_filter_block_control, noc_block_base, fir_filter_block_control::sptr>(
        m, "fir_filter_block_control")
        .def(py::init(&block_controller_factory<fir_filter_block_control>::make_from))
        .def("get_max_num_coefficients",
            &fir_filter_block_control::get_max_num_coefficients)
        .def("set_coefficients", &fir_filter_block_control::set_coefficients)
        .def("get_coefficients", &fir_filter_block_control::get_coefficients);
}

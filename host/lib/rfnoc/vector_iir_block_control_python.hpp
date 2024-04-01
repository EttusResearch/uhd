//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/vector_iir_block_control.hpp>

using namespace uhd::rfnoc;

void export_vector_iir_block_control(py::module& m)
{
    py::class_<vector_iir_block_control, noc_block_base, vector_iir_block_control::sptr>(
        m, "vector_iir_block_control")
        .def(py::init(&block_controller_factory<vector_iir_block_control>::make_from))
        .def("set_alpha", &vector_iir_block_control::set_alpha)
        .def("get_alpha", &vector_iir_block_control::get_alpha)
        .def("set_beta", &vector_iir_block_control::set_beta)
        .def("get_beta", &vector_iir_block_control::get_beta)
        .def("set_delay", &vector_iir_block_control::set_delay)
        .def("get_delay", &vector_iir_block_control::get_delay)
        .def("get_max_delay", &vector_iir_block_control::get_max_delay);
}

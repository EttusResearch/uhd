//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <rfnoc/example/gain_block_control.hpp>

using namespace rfnoc::example;

void export_gain_block_control(py::module& m)
{
    py::class_<gain_block_control, gain_block_control::sptr>(m, "gain_block_control")
        .def(py::init(
            &uhd::rfnoc::block_controller_factory<gain_block_control>::make_from))
        .def("set_gain_value", &gain_block_control::set_gain_value, py::arg("gain"))
        .def("get_gain_value", &gain_block_control::get_gain_value)

        ;
}

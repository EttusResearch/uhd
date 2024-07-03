//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/moving_average_block_control.hpp>

using namespace uhd::rfnoc;

void export_moving_average_block_control(py::module& m)
{
    py::class_<moving_average_block_control,
        noc_block_base,
        moving_average_block_control::sptr>(m, "moving_average_block_control")
        .def(py::init(&block_controller_factory<moving_average_block_control>::make_from))
        .def("set_sum_len", &moving_average_block_control::set_sum_len)
        .def("get_sum_len", &moving_average_block_control::get_sum_len)
        .def("set_divisor", &moving_average_block_control::set_divisor)
        .def("get_divisor", &moving_average_block_control::get_divisor);
}

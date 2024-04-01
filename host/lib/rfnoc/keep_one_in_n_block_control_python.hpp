//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/keep_one_in_n_block_control.hpp>

using namespace uhd::rfnoc;

void export_keep_one_in_n_block_control(py::module& m)
{
    py::class_<keep_one_in_n_block_control,
        noc_block_base,
        keep_one_in_n_block_control::sptr>(m, "keep_one_in_n_block_control")
        .def(py::init(&block_controller_factory<keep_one_in_n_block_control>::make_from))
        .def("get_max_n", &keep_one_in_n_block_control::get_max_n)
        .def("get_n", &keep_one_in_n_block_control::get_n, py::arg("chan") = 0)
        .def("set_n",
            &keep_one_in_n_block_control::set_n,
            py::arg("n"),
            py::arg("chan") = 0)
        .def("get_mode", &keep_one_in_n_block_control::get_mode, py::arg("chan") = 0)
        .def("set_mode",
            &keep_one_in_n_block_control::set_mode,
            py::arg("mode"),
            py::arg("chan") = 0);
}

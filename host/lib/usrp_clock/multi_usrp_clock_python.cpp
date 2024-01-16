//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include "multi_usrp_clock_python.hpp"
#include <uhd/usrp_clock/multi_usrp_clock.hpp>

void export_multi_usrp_clock(py::module& m)
{
    using multi_usrp_clock = uhd::usrp_clock::multi_usrp_clock;

    // clang-format off
    py::class_<multi_usrp_clock, multi_usrp_clock::sptr>(m, "multi_usrp_clock")

        // Factory
        .def(py::init(&multi_usrp_clock::make))

        // General multi_usrp_clock methods
        .def("get_pp_string"           , &multi_usrp_clock::get_pp_string)
        .def("get_num_boards"          , &multi_usrp_clock::get_num_boards)
        .def("get_time"                , &multi_usrp_clock::get_time, py::arg("board") = 0)
        .def("get_sensor"              , &multi_usrp_clock::get_sensor, py::arg("name"), py::arg("board") = 0)
        .def("get_sensor_names"        , &multi_usrp_clock::get_sensor_names, py::arg("board") = 0)
        // clang-format off
        ;
    // clang-format on
}

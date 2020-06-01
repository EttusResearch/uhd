//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "block_controller_factory_python.hpp"
#include <uhd/rfnoc/null_block_control.hpp>

using namespace uhd::rfnoc;

void export_null_block_control(py::module& m)
{
    py::class_<null_block_control, noc_block_base, null_block_control::sptr>(
        m, "null_block_control")
        .def(py::init(&block_controller_factory<null_block_control>::make_from))
        .def("issue_stream_cmd", &null_block_control::issue_stream_cmd)
        .def("reset_counters", &null_block_control::reset_counters)
        .def("set_bytes_per_packet", &null_block_control::set_bytes_per_packet)
        .def("set_throttle_cycles", &null_block_control::set_throttle_cycles)
        .def("get_lines_per_packet", &null_block_control::get_lines_per_packet)
        .def("get_bytes_per_packet", &null_block_control::get_bytes_per_packet)
        .def("get_throttle_cycles", &null_block_control::get_throttle_cycles)
        .def("get_count", &null_block_control::get_count);
}

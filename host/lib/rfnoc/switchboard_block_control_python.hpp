//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/switchboard_block_control.hpp>

using namespace uhd::rfnoc;

void export_switchboard_block_control(py::module& m)
{
    py::class_<switchboard_block_control,
        noc_block_base,
        switchboard_block_control::sptr>(m, "switchboard_block_control")
        .def(py::init(&block_controller_factory<switchboard_block_control>::make_from))
        .def("connect", &switchboard_block_control::connect);
}

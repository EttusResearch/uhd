//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/lc_block_control.hpp>

using namespace uhd::rfnoc;

void export_lc_block_control(py::module& m)
{
    py::class_<lc_block_control, noc_block_base, lc_block_control::sptr>(
        m, "lc_block_control")
        .def(py::init(&block_controller_factory<lc_block_control>::make_from))
        .def("load_key", &lc_block_control::load_key)
        .def("get_feature_ids", &lc_block_control::get_feature_ids);
}

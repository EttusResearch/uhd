//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/fosphor_block_control.hpp>

using namespace uhd::rfnoc;

void export_fosphor_block_control(py::module& m)
{
    py::enum_<fosphor_waterfall_mode>(m, "fosphor_waterfall_mode")
        .value("MAX_HOLD", fosphor_waterfall_mode::MAX_HOLD)
        .value("AVERAGE", fosphor_waterfall_mode::AVERAGE)
        .export_values();

    py::enum_<fosphor_waterfall_predivision_ratio>(
        m, "fosphor_waterfall_predivision_ratio")
        .value("RATIO_1_1", fosphor_waterfall_predivision_ratio::RATIO_1_1)
        .value("RATIO_1_8", fosphor_waterfall_predivision_ratio::RATIO_1_8)
        .value("RATIO_1_64", fosphor_waterfall_predivision_ratio::RATIO_1_64)
        .value("RATIO_1_256", fosphor_waterfall_predivision_ratio::RATIO_1_256)
        .export_values();

    py::class_<fosphor_block_control, noc_block_base, fosphor_block_control::sptr>(
        m, "fosphor_block_control")
        .def(py::init(&block_controller_factory<fosphor_block_control>::make_from))
        .def("set_enable_histogram", &fosphor_block_control::set_enable_histogram)
        .def("get_enable_histogram", &fosphor_block_control::get_enable_histogram)
        .def("set_enable_waterfall", &fosphor_block_control::set_enable_waterfall)
        .def("get_enable_waterfall", &fosphor_block_control::get_enable_waterfall)
        .def("clear_history", &fosphor_block_control::clear_history)
        .def("set_enable_dither", &fosphor_block_control::set_enable_dither)
        .def("get_enable_dither", &fosphor_block_control::get_enable_dither)
        .def("set_enable_noise", &fosphor_block_control::set_enable_noise)
        .def("get_enable_noise", &fosphor_block_control::get_enable_noise)
        .def("set_histogram_decimation", &fosphor_block_control::set_histogram_decimation)
        .def("get_histogram_decimation", &fosphor_block_control::get_histogram_decimation)
        .def("set_histogram_offset", &fosphor_block_control::set_histogram_offset)
        .def("get_histogram_offset", &fosphor_block_control::get_histogram_offset)
        .def("set_histogram_scale", &fosphor_block_control::set_histogram_scale)
        .def("get_histogram_scale", &fosphor_block_control::get_histogram_scale)
        .def("set_histogram_rise_rate", &fosphor_block_control::set_histogram_rise_rate)
        .def("get_histogram_rise_rate", &fosphor_block_control::get_histogram_rise_rate)
        .def("set_histogram_decay_rate", &fosphor_block_control::set_histogram_decay_rate)
        .def("get_histogram_decay_rate", &fosphor_block_control::get_histogram_decay_rate)
        .def("set_spectrum_alpha", &fosphor_block_control::set_spectrum_alpha)
        .def("get_spectrum_alpha", &fosphor_block_control::get_spectrum_alpha)
        .def("set_spectrum_max_hold_decay",
            &fosphor_block_control::set_spectrum_max_hold_decay)
        .def("get_spectrum_max_hold_decay",
            &fosphor_block_control::get_spectrum_max_hold_decay)
        .def("set_waterfall_predivision",
            &fosphor_block_control::set_waterfall_predivision)
        .def("get_waterfall_predivision",
            &fosphor_block_control::get_waterfall_predivision)
        .def("set_waterfall_mode", &fosphor_block_control::set_waterfall_mode)
        .def("get_waterfall_mode", &fosphor_block_control::get_waterfall_mode)
        .def("set_waterfall_decimation", &fosphor_block_control::set_waterfall_decimation)
        .def(
            "get_waterfall_decimation", &fosphor_block_control::get_waterfall_decimation);
}

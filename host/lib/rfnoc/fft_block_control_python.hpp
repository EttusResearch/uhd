//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/fft_block_control.hpp>

using namespace uhd::rfnoc;

void export_fft_block_control(py::module& m)
{
    py::enum_<fft_shift>(m, "fft_shift")
        .value("NORMAL", fft_shift::NORMAL)
        .value("REVERSE", fft_shift::REVERSE)
        .value("NATURAL", fft_shift::NATURAL)
        .export_values();

    py::enum_<fft_direction>(m, "fft_direction")
        .value("REVERSE", fft_direction::REVERSE)
        .value("FORWARD", fft_direction::FORWARD)
        .export_values();

    py::enum_<fft_magnitude>(m, "fft_magnitude")
        .value("COMPLEX", fft_magnitude::COMPLEX)
        .value("MAGNITUDE", fft_magnitude::MAGNITUDE)
        .value("MAGNITUDE_SQUARED", fft_magnitude::MAGNITUDE_SQUARED)
        .export_values();

    py::class_<fft_block_control, noc_block_base, fft_block_control::sptr>(
        m, "fft_block_control")
        .def(py::init(&block_controller_factory<fft_block_control>::make_from))
        .def("set_direction", &fft_block_control::set_direction)
        .def("get_direction", &fft_block_control::get_direction)
        .def("set_magnitude", &fft_block_control::set_magnitude)
        .def("get_magnitude", &fft_block_control::get_magnitude)
        .def("set_shift_config", &fft_block_control::set_shift_config)
        .def("get_shift_config", &fft_block_control::get_shift_config)
        .def("set_scaling", &fft_block_control::set_scaling)
        .def("get_scaling", &fft_block_control::get_scaling)
        .def("set_length", &fft_block_control::set_length)
        .def("get_length", &fft_block_control::get_length);
}

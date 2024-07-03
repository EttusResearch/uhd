//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/block_controller_factory_python.hpp>
#include <uhd/rfnoc/siggen_block_control.hpp>

using namespace uhd::rfnoc;

void export_siggen_block_control(py::module& m)
{
    py::enum_<siggen_waveform>(m, "siggen_waveform")
        .value("CONSTANT", siggen_waveform::CONSTANT)
        .value("SINE_WAVE", siggen_waveform::SINE_WAVE)
        .value("NOISE", siggen_waveform::NOISE)
        .export_values();

    py::class_<siggen_block_control, noc_block_base, siggen_block_control::sptr>(
        m, "siggen_block_control")
        .def(py::init(&block_controller_factory<siggen_block_control>::make_from))
        .def("set_enable", &siggen_block_control::set_enable)
        .def("get_enable", &siggen_block_control::get_enable)
        .def("set_waveform", &siggen_block_control::set_waveform)
        .def("get_waveform", &siggen_block_control::get_waveform)
        .def("set_amplitude", &siggen_block_control::set_amplitude)
        .def("get_amplitude", &siggen_block_control::get_amplitude)
        .def("set_constant", &siggen_block_control::set_constant)
        .def("get_constant", &siggen_block_control::get_constant)
        .def("set_sine_phase_increment", &siggen_block_control::set_sine_phase_increment)
        .def("get_sine_phase_increment", &siggen_block_control::get_sine_phase_increment)
        .def("set_sine_frequency", &siggen_block_control::set_sine_frequency)
        .def("set_samples_per_packet", &siggen_block_control::set_samples_per_packet)
        .def("get_samples_per_packet", &siggen_block_control::get_samples_per_packet);
}

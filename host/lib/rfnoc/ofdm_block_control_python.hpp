//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "block_controller_factory_python.hpp"
#include <uhd/rfnoc/ofdm_block_control.hpp>

using namespace uhd::rfnoc;

void export_ofdm_block_control(py::module& m)
{
    py::enum_<ofdm_direction>(m, "ofdm_direction")
        .value("REVERSE", ofdm_direction::REVERSE)
        .value("FORWARD", ofdm_direction::FORWARD)
        .export_values();

    py::class_<ofdm_block_control, noc_block_base, ofdm_block_control::sptr>(
        m, "ofdm_block_control")
        .def(py::init(&block_controller_factory<ofdm_block_control>::make_from))
        .def("write_reg", &ofdm_block_control::write_reg)
        .def("read_reg", &ofdm_block_control::read_reg)
        .def("write_and_check_reg", &ofdm_block_control::write_and_check_reg)
        .def("set_fft_size", &ofdm_block_control::set_fft_size)
        .def("get_fft_size", &ofdm_block_control::get_fft_size)
        .def("set_fft_scaling", &ofdm_block_control::set_fft_scaling)
        .def("get_fft_scaling", &ofdm_block_control::get_fft_scaling)
        .def("set_fft_direction", &ofdm_block_control::set_fft_direction)
        .def("get_fft_direction", &ofdm_block_control::get_fft_direction)
        .def("get_max_fft_size", &ofdm_block_control::get_max_fft_size)
        .def("get_max_cp_length", &ofdm_block_control::get_max_cp_length)
        .def("get_max_cp_rem_list_length", &ofdm_block_control::get_max_cp_rem_list_length)
        .def("get_max_cp_ins_list_length", &ofdm_block_control::get_max_cp_ins_list_length)
        .def("load_cp_insertion_fifo", &ofdm_block_control::load_cp_insertion_fifo)
        .def("clear_cp_insertion_fifo", &ofdm_block_control::clear_cp_insertion_fifo)
        .def("load_cp_removal_fifo", &ofdm_block_control::load_cp_removal_fifo)
        .def("clear_cp_removal_fifo", &ofdm_block_control::clear_cp_removal_fifo)
        ;
}

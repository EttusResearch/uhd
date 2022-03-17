//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "block_controller_factory_python.hpp"
#include <uhd/rfnoc/replay_block_control.hpp>

using namespace uhd::rfnoc;

void export_replay_block_control(py::module& m)
{
    py::class_<replay_block_control, noc_block_base, replay_block_control::sptr>(
        m, "replay_block_control")
        .def(py::init(&block_controller_factory<replay_block_control>::make_from))
        .def("record", &replay_block_control::record)
        .def("record_restart", &replay_block_control::record_restart)
        .def("play", &replay_block_control::play)
        .def("stop", &replay_block_control::stop)
        .def("get_mem_size", &replay_block_control::get_mem_size)
        .def("get_word_size", &replay_block_control::get_word_size)
        .def("get_record_offset", &replay_block_control::get_record_offset)
        .def("get_record_size", &replay_block_control::get_record_size)
        .def("get_record_fullness", &replay_block_control::get_record_fullness)
        .def("get_record_type", &replay_block_control::get_record_type)
        .def("get_record_item_size", &replay_block_control::get_record_item_size)
        // The "pass object into function to fill if available" is very un-Pythonic,
        // and really is more of a C thing than a C++ thing. In the binding, we
        // simply return None or the thing that we want, it it's available.
        .def(
            "get_record_async_metadata",
            [](replay_block_control& self, const double timeout) -> py::object {
                uhd::rx_metadata_t md;
                if (self.get_record_async_metadata(md, timeout)) {
                    return py::cast(md);
                }
                return py::cast(nullptr);
            },
            py::arg("timeout") = 0.1)
        .def("get_play_offset", &replay_block_control::get_play_offset)
        .def("get_play_size", &replay_block_control::get_play_size)
        .def("get_max_items_per_packet", &replay_block_control::get_max_items_per_packet)
        .def("get_max_packet_size", &replay_block_control::get_max_packet_size)
        .def("get_play_type", &replay_block_control::get_play_type)
        .def("get_play_item_size", &replay_block_control::get_play_item_size)
        // See comment on get_record_async_metadata()
        .def(
            "get_play_async_metadata",
            [](replay_block_control& self, const double timeout) -> py::object {
                uhd::async_metadata_t md;
                if (self.get_play_async_metadata(md, timeout)) {
                    return py::cast(md);
                }
                return py::cast(nullptr);
            },
            py::arg("timeout") = 0.1)
        .def("set_record_type", &replay_block_control::set_record_type)
        .def("config_play", &replay_block_control::config_play)
        .def("set_play_type", &replay_block_control::set_play_type)
        .def("set_max_items_per_packet", &replay_block_control::set_max_items_per_packet)
        .def("set_max_packet_size", &replay_block_control::set_max_packet_size)
        .def("issue_stream_cmd", &replay_block_control::issue_stream_cmd);
}

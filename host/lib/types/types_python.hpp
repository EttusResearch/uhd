//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_PYTHON_HPP
#define INCLUDED_UHD_TYPES_PYTHON_HPP

#include <uhd/types/device_addr.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <pybind11/stl.h>
#include <map>
#include <string>


void export_types(py::module& m)
{
    using stream_cmd_t  = uhd::stream_cmd_t;
    using stream_mode_t = stream_cmd_t::stream_mode_t;
    using str_map       = std::map<std::string, std::string>;

    py::enum_<stream_mode_t>(m, "stream_mode")
        .value("start_cont", stream_cmd_t::STREAM_MODE_START_CONTINUOUS)
        .value("stop_cont", stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS)
        .value("num_done", stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE)
        .value("num_more", stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE);

    py::class_<stream_cmd_t>(m, "stream_cmd")
        .def(py::init<stream_cmd_t::stream_mode_t>())
        // Properties
        .def_readwrite("num_samps", &stream_cmd_t::num_samps)
        .def_readwrite("time_spec", &stream_cmd_t::time_spec)
        .def_readwrite("stream_now", &stream_cmd_t::stream_now);

    py::class_<uhd::device_addr_t>(m, "device_addr")
        // Constructors
        .def(py::init<>())
        .def(py::init<std::string>())
        .def(py::init<str_map>())

        // Methods
        .def("__str__", &uhd::device_addr_t::to_pp_string)
        .def("to_string", &uhd::device_addr_t::to_string)
        .def("to_pp_string", &uhd::device_addr_t::to_pp_string)
        .def("to_dict", [](uhd::device_addr_t& self) {
            return static_cast<std::map<std::string, std::string>>(self);
        });
    // This will allow functions in Python that take a device_addr to also take
    // a string:
    py::implicitly_convertible<std::string, uhd::device_addr_t>();
}

#endif /* INCLUDED_UHD_TYPES_PYTHON_HPP */

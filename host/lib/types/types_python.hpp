//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_PYTHON_HPP
#define INCLUDED_UHD_TYPES_PYTHON_HPP

#include <uhd/types/device_addr.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/direction.hpp>
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
        .def_readwrite("stream_mode", &stream_cmd_t::stream_mode)
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
        .def("to_dict",
            [](uhd::device_addr_t& self) {
                return static_cast<std::map<std::string, std::string>>(self);
            })
        .def("keys", [](const uhd::device_addr_t& self) { return self.keys(); })
        .def("values", [](const uhd::device_addr_t& self) { return self.vals(); })
        .def("get",
            [](const uhd::device_addr_t& self, const std::string& key) -> py::object {
                if (self.has_key(key)) {
                    return py::str(self.get(key));
                }
                return py::none();
            })
        .def("get",
            [](const uhd::device_addr_t& self,
                const std::string& key,
                const std::string& other) { return self.get(key, other); })
        .def(
            "pop",
            [](uhd::device_addr_t& self,
                const std::string& key,
                const char* def) -> std::string {
                if (!self.has_key(key)) {
                    if (def == nullptr) {
                        throw py::key_error(key);
                    }
                    return std::string(def);
                }
                return self.pop(key);
            },
            py::arg("k"),
            py::arg("d") = nullptr)
        .def(
            "update",
            [](uhd::device_addr_t& self,
                const uhd::device_addr_t& new_dict,
                const bool fail_on_conflict) { self.update(new_dict, fail_on_conflict); },
            py::arg("E"),
            py::arg("fail_on_conflict") = true)
        .def(
            "update",
            [](uhd::device_addr_t& self,
                const std::map<std::string, std::string>& new_dict,
                const bool fail_on_conflict) {
                self.update(uhd::device_addr_t(new_dict), fail_on_conflict);
            },
            py::arg("E"),
            py::arg("fail_on_conflict") = true)
        .def("separate",
            [](const uhd::device_addr_t& self) {
                return uhd::separate_device_addr(self);
            })
        .def("__getitem__",
            [](const uhd::device_addr_t& self, const std::string& key) -> std::string {
                if (!self.has_key(key)) {
                    throw py::key_error(key);
                }
                return self.get(key);
            })
        .def("__setitem__",
            [](uhd::device_addr_t& self, const std::string& key, const std::string& val) {
                self.set(key, val);
            })
        .def("__contains__",
            [](const uhd::device_addr_t& self, const std::string& key) {
                return self.has_key(key);
            })
        .def("__len__", [](const uhd::device_addr_t& self) { return self.size(); })
        .def("__eq__",
            [](const uhd::device_addr_t& self, const uhd::device_addr_t& other) {
                return self == other;
            })
        .def("__ne__",
            [](const uhd::device_addr_t& self, const uhd::device_addr_t& other) {
                return self != other;
            });
    // This will allow functions in Python that take a device_addr to also take
    // a string:
    py::implicitly_convertible<std::string, uhd::device_addr_t>();

    m.def("separate_device_addr", &uhd::separate_device_addr);
    m.def("combine_device_addrs", &uhd::combine_device_addrs);

    py::enum_<uhd::direction_t>(m, "direction_t")
        .value("RX_DIRECTION", uhd::direction_t::RX_DIRECTION)
        .value("TX_DIRECTION", uhd::direction_t::TX_DIRECTION)
        .value("DX_DIRECTION", uhd::direction_t::DX_DIRECTION)


        ;
}

#endif /* INCLUDED_UHD_TYPES_PYTHON_HPP */

//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_METADATA_PYTHON_HPP
#define INCLUDED_UHD_METADATA_PYTHON_HPP

#include <uhd/types/ranges.hpp>

void export_metadata(py::module& m)
{
    using range_t          = uhd::range_t;
    using meta_range_t     = uhd::meta_range_t;
    using rx_metadata_t    = uhd::rx_metadata_t;
    using error_code_t     = rx_metadata_t::error_code_t;
    using tx_metadata_t    = uhd::tx_metadata_t;
    using async_metadata_t = uhd::async_metadata_t;
    using event_code_t     = async_metadata_t::event_code_t;

    py::enum_<error_code_t>(m, "rx_metadata_error_code")
        .value("none", error_code_t::ERROR_CODE_NONE)
        .value("timeout", error_code_t::ERROR_CODE_TIMEOUT)
        .value("late", error_code_t::ERROR_CODE_LATE_COMMAND)
        .value("broken_chain", error_code_t::ERROR_CODE_BROKEN_CHAIN)
        .value("overflow", error_code_t::ERROR_CODE_OVERFLOW)
        .value("alignment", error_code_t::ERROR_CODE_ALIGNMENT)
        .value("bad_packet", error_code_t::ERROR_CODE_BAD_PACKET);

    py::class_<range_t>(m, "range")
        // Constructors
        .def(py::init<double>())
        .def(py::init<double, double>())
        .def(py::init<double, double, double>())

        // Methods
        .def("start", &range_t::start)
        .def("stop", &range_t::stop)
        .def("step", &range_t::step)
        .def("__str__", &range_t::to_pp_string);

    py::class_<meta_range_t>(m, "meta_range_t")
        // Constructors
        .def(py::init<>())
        .def(py::init<double, double>())
        .def(py::init<double, double, double>())

        // Methods
        .def("start", &meta_range_t::start)
        .def("stop", &meta_range_t::stop)
        .def("step", &meta_range_t::step)
        .def("clip", &meta_range_t::clip, py::arg("value"), py::arg("clip_step") = false)
        .def("__str__", &meta_range_t::to_pp_string);

    py::class_<rx_metadata_t>(m, "rx_metadata")
        .def(py::init<>())

        // Methods
        .def("reset", &rx_metadata_t::reset)
        .def("to_pp_string", &rx_metadata_t::to_pp_string)
        .def("strerror", &rx_metadata_t::strerror)
        .def("__str__", &rx_metadata_t::to_pp_string, py::arg("compact") = false)

        // Properties
        .def_readonly("has_time_spec", &rx_metadata_t::has_time_spec)
        .def_readonly("time_spec", &rx_metadata_t::time_spec)
        .def_readonly("more_fragments", &rx_metadata_t::more_fragments)
        .def_readonly("start_of_burst", &rx_metadata_t::start_of_burst)
        .def_readonly("end_of_burst", &rx_metadata_t::end_of_burst)
        .def_readonly("error_code", &rx_metadata_t::error_code)
        .def_readonly("out_of_sequence", &rx_metadata_t::out_of_sequence);

    py::class_<tx_metadata_t>(m, "tx_metadata")
        .def(py::init<>())

        // Properties
        .def_readwrite("has_time_spec", &tx_metadata_t::has_time_spec)
        .def_readwrite("time_spec", &tx_metadata_t::time_spec)
        .def_readwrite("start_of_burst", &tx_metadata_t::start_of_burst)
        .def_readwrite("end_of_burst", &tx_metadata_t::end_of_burst);

    py::enum_<event_code_t>(m, "tx_metadata_event_code")
        .value("burst_ack", event_code_t::EVENT_CODE_BURST_ACK)
        .value("underflow", event_code_t::EVENT_CODE_UNDERFLOW)
        .value("seq_error", event_code_t::EVENT_CODE_SEQ_ERROR)
        .value("time_error", event_code_t::EVENT_CODE_TIME_ERROR)
        .value("underflow_in_packet", event_code_t::EVENT_CODE_UNDERFLOW_IN_PACKET)
        .value("seq_error_in_packet", event_code_t::EVENT_CODE_SEQ_ERROR_IN_BURST)
        .value("user_payload", event_code_t::EVENT_CODE_USER_PAYLOAD);

    py::class_<async_metadata_t>(m, "async_metadata")
        .def(py::init<>())

        // Properties
        .def_readonly("channel", &async_metadata_t::channel)
        .def_readonly("has_time_spec", &async_metadata_t::has_time_spec)
        .def_readonly("time_spec", &async_metadata_t::time_spec)
        .def_readonly("event_code", &async_metadata_t::event_code)
        // TODO: Expose user payloads
        //.def_readonly("user_payload" , &async_metadata_t::user_payload )
        ;
}

#endif /* INCLUDED_UHD_METADATA_PYTHON_HPP */

//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TIME_SPEC_PYTHON_HPP
#define INCLUDED_UHD_TIME_SPEC_PYTHON_HPP

#include <uhd/types/time_spec.hpp>
#include <pybind11/operators.h>

void export_time_spec(py::module& m)
{
    using time_spec_t = uhd::time_spec_t;

    py::class_<time_spec_t>(m, "time_spec")
        // Additional constructors
        .def(py::init<double>())
        .def(py::init<int64_t, double>())
        .def(py::init<int64_t, long, double>())

        // Methods
        .def_static("from_ticks", &time_spec_t::from_ticks)

        .def("get_tick_count", &time_spec_t::get_tick_count)
        .def("to_ticks", &time_spec_t::to_ticks)
        .def("get_real_secs", &time_spec_t::get_real_secs)
        .def("get_full_secs", &time_spec_t::get_full_secs)
        .def("get_frac_secs", &time_spec_t::get_frac_secs)

        .def(py::self == py::self)
        .def(py::self != py::self)
        .def(py::self > py::self)
        .def(py::self >= py::self)
        .def(py::self < py::self)
        .def(py::self <= py::self)

        .def(py::self += time_spec_t())
        .def(py::self -= time_spec_t())
        .def(py::self + time_spec_t())
        .def(py::self - time_spec_t())
        .def(py::self += double())
        .def(py::self -= double())
        .def(py::self + double())
        .def(py::self - double());
}

#endif /* INCLUDED_UHD_TIME_SPEC_PYTHON_HPP */

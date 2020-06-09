//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_FILTERS_PYTHON_HPP
#define INCLUDED_UHD_FILTERS_PYTHON_HPP

#include <uhd/types/filters.hpp>

void export_filters(py::module& m)
{
    using filter_info_base   = uhd::filter_info_base;
    using filter_info_type   = filter_info_base::filter_type;
    using analog_filter_base = uhd::analog_filter_base;
    using analog_filter_lp   = uhd::analog_filter_lp;

    py::enum_<filter_info_type>(m, "filter_type")
        .value("analog_low_pass", filter_info_base::ANALOG_LOW_PASS)
        .value("analog_band_pass", filter_info_base::ANALOG_BAND_PASS)
        .value("digital_i16", filter_info_base::DIGITAL_I16)
        .value("digital_fir_i16", filter_info_base::DIGITAL_FIR_I16);

    py::class_<filter_info_base, filter_info_base::sptr>(m, "filter_info_base")
        .def(py::init<filter_info_type, bool, size_t>())

        // Methods
        .def("is_bypassed", &filter_info_base::is_bypassed)
        .def("get_type", &filter_info_base::get_type)
        .def("__str__", &filter_info_base::to_pp_string);

    py::class_<analog_filter_base, filter_info_base, analog_filter_base::sptr>(
        m, "analog_filter_base")
        .def(py::init<filter_info_type, bool, size_t, std::string>())

        // Methods
        .def("get_analog_type", &analog_filter_base::get_analog_type);

    py::class_<analog_filter_lp, analog_filter_base, std::shared_ptr<analog_filter_lp>>(m, "analog_filter_lp")
        .def(
            py::init<filter_info_type, bool, size_t, const std::string, double, double>())

        // Methods
        .def("get_cutoff", &analog_filter_lp::get_cutoff)
        .def("get_rolloff", &analog_filter_lp::get_rolloff)
        .def("set_cutoff", &analog_filter_lp::set_cutoff);
}

#endif /* INCLUDED_UHD_FILTERS_PYTHON_HPP */

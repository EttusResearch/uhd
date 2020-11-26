//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TUNE_PYTHON_HPP
#define INCLUDED_UHD_TUNE_PYTHON_HPP

#include <uhd/types/tune_request.hpp>
#include <uhd/types/tune_result.hpp>

void export_tune(py::module& m)
{
    using tune_request_t = uhd::tune_request_t;
    using tune_result_t  = uhd::tune_result_t;
    using policy_t       = tune_request_t::policy_t;

    py::enum_<policy_t>(m, "tune_request_policy")
        .value("none", tune_request_t::POLICY_NONE)
        .value("auto", tune_request_t::POLICY_AUTO)
        .value("manual", tune_request_t::POLICY_MANUAL);

    py::class_<tune_request_t>(m, "tune_request")
        .def(py::init<double, double>())
        .def(py::init<double>(), py::arg("target_freq") = 0.0)
        .def_readwrite("target_freq", &tune_request_t::target_freq)
        .def_readwrite("rf_freq_policy", &tune_request_t::rf_freq_policy)
        .def_readwrite("dsp_freq_policy", &tune_request_t::dsp_freq_policy)
        .def_readwrite("rf_freq", &tune_request_t::rf_freq)
        .def_readwrite("dsp_freq", &tune_request_t::dsp_freq)
        .def_readwrite("args", &tune_request_t::args);
    py::implicitly_convertible<double, tune_request_t>();

    py::class_<tune_result_t>(m, "tune_result")
        .def(py::init<>())
        .def_readwrite("clipped_rf_freq", &tune_result_t::clipped_rf_freq)
        .def_readwrite("target_rf_freq", &tune_result_t::target_rf_freq)
        .def_readwrite("actual_rf_freq", &tune_result_t::actual_rf_freq)
        .def_readwrite("target_dsp_freq", &tune_result_t::target_dsp_freq)
        .def_readwrite("actual_dsp_freq", &tune_result_t::actual_dsp_freq)
        .def("__str__", &tune_result_t::to_pp_string);
}

#endif /* INCLUDED_UHD_TUNE_PYTHON_HPP */

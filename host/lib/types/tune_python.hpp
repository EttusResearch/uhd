//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TUNE_PYTHON_HPP
#define INCLUDED_UHD_TUNE_PYTHON_HPP

#include <uhd/types/tune_result.hpp>
#include <uhd/types/tune_request.hpp>

void export_tune()
{
    using tune_request_t = uhd::tune_request_t;
    using tune_result_t  = uhd::tune_result_t;
    using policy_t       = tune_request_t::policy_t;

    bp::enum_<policy_t>("tune_request_policy")
        .value("none",   tune_request_t::POLICY_NONE  )
        .value("auto",   tune_request_t::POLICY_AUTO  )
        .value("manual", tune_request_t::POLICY_MANUAL)
        ;

    bp::class_<tune_request_t>("tune_request", bp::init<double>())
        .def(bp::init<double, double>())
        .def_readwrite("target_freq"    , &tune_request_t::target_freq    )
        .def_readwrite("rf_freq_policy" , &tune_request_t::rf_freq_policy )
        .def_readwrite("dsp_freq_policy", &tune_request_t::dsp_freq_policy)
        .def_readwrite("rf_freq"        , &tune_request_t::rf_freq        )
        .def_readwrite("dsp_freq"       , &tune_request_t::dsp_freq       )
        .def_readwrite("args"           , &tune_request_t::args           )
        ;

    bp::class_<tune_result_t>("tune_result", bp::init<>())
        ;
}

#endif /* INCLUDED_UHD_TUNE_PYTHON_HPP */

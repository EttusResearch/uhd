//
// Copyright 2017 Ettus Research
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_PYTHON_HPP
#define INCLUDED_UHD_TYPES_PYTHON_HPP

#include <uhd/types/stream_cmd.hpp>

void export_types()
{
    using stream_cmd_t  = uhd::stream_cmd_t;
    using stream_mode_t = stream_cmd_t::stream_mode_t;

    bp::enum_<stream_mode_t>("stream_mode")
        .value("start_cont", stream_cmd_t::STREAM_MODE_START_CONTINUOUS  )
        .value("stop_cont" , stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS   )
        .value("num_done"  , stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE)
        .value("num_more"  , stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE)
        ;

    bp::class_<stream_cmd_t>("stream_cmd", bp::init<stream_cmd_t::stream_mode_t>())

        // Properties
        .def_readwrite("num_samps" , &stream_cmd_t::num_samps )
        .def_readwrite("time_spec" , &stream_cmd_t::time_spec )
        .def_readwrite("stream_now", &stream_cmd_t::stream_now)
        ;
}

#endif /* INCLUDED_UHD_TYPES_PYTHON_HPP */

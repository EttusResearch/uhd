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

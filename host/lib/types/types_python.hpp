//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_PYTHON_HPP
#define INCLUDED_UHD_TYPES_PYTHON_HPP

#include <uhd/types/device_addr.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <boost/python.hpp>
#include <string>
#include <map>


//! Make a device_addr_t from a Python dict of strings
static uhd::device_addr_t make_device_addr(bp::dict& info) {
    // Manually extract each key and each value, copy them to a map of strings, and return that.
    std::map<std::string,std::string> info_map;
    auto keys = info.keys();
    for (int ii = 0; ii < bp::len(keys); ++ii) {
        std::string key = bp::extract<std::string>(keys[ii]);
        info_map[key] = bp::extract<std::string>(info[key]);
    }
    return uhd::device_addr_t(info_map);
}

void export_types()
{
    using stream_cmd_t  = uhd::stream_cmd_t;
    using stream_mode_t = stream_cmd_t::stream_mode_t;
    using str_map = std::map<std::string, std::string>;

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

    bp::class_<uhd::device_addr_t>("device_addr", bp::init<bp::optional<std::string> >())
        // Constructors
        /* TODO: This calls the correct C++ constructor, but Python
                 dictionaries != str_maps, so we get a signature error */
        .def(bp::init<str_map>())

        // Methods
        .def("__str__", &uhd::device_addr_t::to_pp_string)
        .def("to_string", &uhd::device_addr_t::to_string)
        .def("to_pp_string", &uhd::device_addr_t::to_pp_string)
        ;

    bp::def("make_device_addr", make_device_addr);
}

#endif /* INCLUDED_UHD_TYPES_PYTHON_HPP */

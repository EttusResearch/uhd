//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "lib_helper.hpp"
#include "converters.hpp"
#include "../lib/udev_helper.hpp"
#include "../lib/net_helper.hpp"
#include <boost/python.hpp>

namespace bp = boost::python;

void export_helper(){
    //Register submodule types
    bp::object helper_module(bp::handle<>(bp::borrowed(PyImport_AddModule("libpyusrp_periphs.helper"))));
    bp::scope().attr("helper") = helper_module;
    bp::scope io_scope = helper_module;

    bp::class_<mpm::udev_helper>("udev_helper", bp::init<>())
        .def("get_eeprom", &mpm::udev_helper::get_eeprom)
        ;
    bp::to_python_converter<std::vector< std::string >, iterable_to_python_list<std::vector< std::string > >, false>();
}


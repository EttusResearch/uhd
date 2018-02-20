//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "tests_periphs.hpp"
#include <mpm/tests/tests_spi_iface.hpp>
#include <mpm/spi_iface.hpp>
#include <boost/python.hpp>

namespace bp = boost::python;

void export_tests(){
    //Register submodule types
    bp::object tests_module(bp::handle<>(bp::borrowed(PyImport_AddModule("libpyusrp_periphs.tests"))));
    bp::scope().attr("tests") = tests_module;
    bp::scope io_scope = tests_module;

    bp::class_<mpm::tests_spi_iface, bp::bases<mpm::spi_iface>, boost::shared_ptr<mpm::tests_spi_iface> >("test_spi_iface", bp::init<>())
        .def("make", &mpm::tests_spi_iface::make)
        .staticmethod("make")
        ;
}

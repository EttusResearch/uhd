//
// Copyright 2017 Ettus Research, National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

// include hackery to only include boost python and define the macro here
#include <boost/python.hpp>
#define LIBMPM_PYTHON
#define LIBMPM_BOOST_PREAMBLE(module)  \
    /* Register submodule types */      \
    namespace bp = boost::python; \
    bp::object py_module(bp::handle<>(bp::borrowed(PyImport_AddModule("libpyusrp_periphs." module)))); \
    bp::scope().attr(module) = py_module; \
    bp::scope io_scope = py_module;

//#include "types.hpp"
#include "converters.hpp"
#include <mpm/xbar_iface.hpp>
#include <mpm/types/types_python.hpp>
#include <mpm/spi/spi_python.hpp>
#include <mpm/ad937x/ad937x_ctrl.hpp>
#include <mpm/dboards/magnesium_manager.hpp>
#include <boost/noncopyable.hpp>

namespace bp = boost::python;

BOOST_PYTHON_MODULE(libpyusrp_periphs)
{
    bp::object package = bp::scope();
    package.attr("__path__") = "libpyusrp_periphs";
    export_converter();
    export_types();
    export_spi();
    export_mykonos();
    export_xbar();
    export_magnesium();
}

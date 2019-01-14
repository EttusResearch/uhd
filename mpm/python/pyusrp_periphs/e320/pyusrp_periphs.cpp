//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

// include hackery to only include boost python and define the macro here
#include <boost/python.hpp>
#define LIBMPM_PYTHON
#define LIBMPM_BOOST_PREAMBLE(module)                                                 \
    /* Register submodule types */                                                    \
    namespace bp = boost::python;                                                     \
    bp::object py_module(                                                             \
        bp::handle<>(bp::borrowed(PyImport_AddModule("libpyusrp_periphs." module)))); \
    bp::scope().attr(module) = py_module;                                             \
    bp::scope io_scope       = py_module;

#include "../converters.hpp"
#include <mpm/ad9361/ad9361_ctrl.hpp>
#include <mpm/dboards/neon_manager.hpp>
#include <mpm/spi/spi_python.hpp>
#include <mpm/types/types_python.hpp>
#include <mpm/xbar_iface.hpp>
#include <boost/noncopyable.hpp>

namespace bp = boost::python;

BOOST_PYTHON_MODULE(libpyusrp_periphs)
{
    bp::object package       = bp::scope();
    package.attr("__path__") = "libpyusrp_periphs";
    export_converter();
    export_types();
    export_spi();
    export_xbar();
    export_catalina();
    export_neon();
}

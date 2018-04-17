//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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

//! RAII-style GIL release method
//
// To release the GIL using this method, simply instantiate this class in the
// scope that needs to release the GIL.
//
// Note that using this class assumes that threads have already been
// initialized. See also https://docs.python.org/3.5/c-api/init.html for more
// documentation on Python initialization and threads.
class scoped_gil_release
{
public:
    inline scoped_gil_release()
    {
        _thread_state = PyEval_SaveThread();
    }

    inline ~scoped_gil_release()
    {
        PyEval_RestoreThread(_thread_state);
        _thread_state = nullptr;
    }

private:
    PyThreadState* _thread_state;
};

//#include "types.hpp"
#include "../converters.hpp"
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

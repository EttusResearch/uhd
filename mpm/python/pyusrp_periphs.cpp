//
// Copyright 2017 Ettus Research (National Instruments)
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
#include "mpm/xbar_iface.hpp"
#include <mpm/ad937x/ad937x_ctrl.hpp>
#include <mpm/lmk04828//lmk04828_spi_iface.hpp>
#include "mpm/dboards/magnesium_manager.hpp"
//#include "lib_periphs.hpp"
//#include "dboards.hpp"
#include <boost/noncopyable.hpp>

namespace bp = boost::python;

BOOST_PYTHON_MODULE(libpyusrp_periphs)
{
    bp::object package = bp::scope();
    package.attr("__path__") = "libpyusrp_periphs";
    export_converter();
    //export_types();
    //export_spi();
    export_lmk();
    export_mykonos();
    export_xbar();
    export_dboards();
}

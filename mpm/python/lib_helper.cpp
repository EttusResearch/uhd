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
    bp::def("get_if_addrs", &mpm::network::get_if_addrs);
    bp::to_python_converter<std::vector< std::string >, iterable_to_python_list<std::vector< std::string > >, false>();
}


//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "property_tree_python.hpp"
#include <uhd/property_tree.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/usrp/dboard_iface.hpp>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <string>

namespace py = pybind11;

template <typename T>
void export_property(py::module& m, const std::string& type_str)
{
    const std::string classname = std::string("property__") + type_str;
    py::class_<uhd::property<T>>(m, classname.c_str())
        .def("get", &uhd::property<T>::get)
        .def("get_desired", &uhd::property<T>::get_desired)
        .def("set", &uhd::property<T>::set)
        .def("set_coerced", &uhd::property<T>::set_coerced);
}

void export_property_tree(py::module& m)
{
    using property_tree = uhd::property_tree;
    using fs_path       = uhd::fs_path;

    py::class_<fs_path>(m, "fs_path")
        // Constructors
        .def(py::init<>())
        .def(py::init<std::string>());
    py::implicitly_convertible<std::string, fs_path>();

    // Per type we want to expose, add one line here, and one accessor below
    export_property<int>(m, "int");
    export_property<double>(m, "double");
    export_property<std::string>(m, "str");
    export_property<bool>(m, "bool");
    export_property<uhd::device_addr_t>(m, "device_addr");
    export_property<uhd::usrp::dboard_iface::sptr>(m, "dboard_iface");

    py::class_<property_tree>(m, "property_tree")
        .def("subtree", &property_tree::subtree, py::arg("path"))
        .def("exists", &property_tree::exists, py::arg("path"))
        .def("list", &property_tree::list, py::arg("path"))
        // One line per type
        .def(
            "access_int", &property_tree::access<int>, py::return_value_policy::reference)
        .def("access_double",
            &property_tree::access<double>,
            py::return_value_policy::reference)
        .def("access_str",
            &property_tree::access<std::string>,
            py::return_value_policy::reference)
        .def("access_bool",
            &property_tree::access<bool>,
            py::return_value_policy::reference)
        .def("access_device_addr",
            &property_tree::access<uhd::device_addr_t>,
            py::return_value_policy::reference)
        .def("access_dboard_iface",
            &property_tree::access<uhd::usrp::dboard_iface::sptr>,
            py::return_value_policy::reference)
        // End of types
        ;
}

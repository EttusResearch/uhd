//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_SUBDEV_SPEC_PYTHON_HPP
#define INCLUDED_UHD_USRP_SUBDEV_SPEC_PYTHON_HPP

#include <uhd/usrp/subdev_spec.hpp>

void export_subdev_spec(py::module& m)
{
    using subdev_spec_pair_t = uhd::usrp::subdev_spec_pair_t;
    using subdev_spec_t      = uhd::usrp::subdev_spec_t;

    py::class_<subdev_spec_pair_t>(m, "subdev_spec_pair")
        .def(py::init<const std::string&, const std::string&>())
        // Properties
        .def_readwrite("db_name", &subdev_spec_pair_t::db_name)
        .def_readwrite("sd_name", &subdev_spec_pair_t::sd_name);

    py::class_<subdev_spec_t>(m, "subdev_spec")
        .def(py::init<const std::string&>())

        // Methods
        .def("__str__", &subdev_spec_t::to_pp_string)
        .def("to_string", &subdev_spec_t::to_string)
        .def("__getitem__",
            [](subdev_spec_t& self, const size_t idx) { return self.at(idx); })

        ;
}

#endif /* INCLUDED_UHD_USRP_SUBDEV_SPEC_PYTHON_HPP */

//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_SUBDEV_SPEC_PYTHON_HPP
#define INCLUDED_UHD_USRP_SUBDEV_SPEC_PYTHON_HPP

#include <uhd/usrp/subdev_spec.hpp>

void export_subdev_spec()
{
    using subdev_spec_pair_t = uhd::usrp::subdev_spec_pair_t;
    using subdev_spec_t      = uhd::usrp::subdev_spec_t;

    bp::class_<subdev_spec_pair_t>
        ("subdev_spec_pair", bp::init<const std::string&, const std::string &>())

        // Properties
        .add_property("db_name", &subdev_spec_pair_t::db_name)
        .add_property("sd_name", &subdev_spec_pair_t::sd_name)
        ;

    bp::class_<std::vector<subdev_spec_pair_t> >("subdev_spec_vector")
        .def(bp::vector_indexing_suite<std::vector<subdev_spec_pair_t> >());

    bp::class_<subdev_spec_t, bp::bases<std::vector<subdev_spec_pair_t> > >
        ("subdev_spec", bp::init<const std::string &>())

        // Methods
        .def("__str__",   &subdev_spec_t::to_pp_string)
        .def("to_string", &subdev_spec_t::to_string)
        ;
}

#endif /* INCLUDED_UHD_USRP_SUBDEV_SPEC_PYTHON_HPP */

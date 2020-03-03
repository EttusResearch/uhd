//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_FE_CONNECTION_PYTHON_HPP
#define INCLUDED_UHD_USRP_FE_CONNECTION_PYTHON_HPP

#include <uhd/usrp/fe_connection.hpp>

void export_fe_connection(py::module& m)
{
    using fe_connection_t = uhd::usrp::fe_connection_t;
    using sampling_t      = fe_connection_t::sampling_t;

    py::enum_<sampling_t>(m, "sampling")
        .value("QUADRATURE", sampling_t::QUADRATURE)
        .value("HETERODYNE", sampling_t::HETERODYNE)
        .value("REAL", sampling_t::REAL);

    py::class_<fe_connection_t>(m, "fe_connection")

        // Constructors
        .def(py::init<sampling_t, bool, bool, bool, double>())
        .def(py::init<const std::string&, double>())

        // Methods
        .def("get_sampling_mode", &fe_connection_t::get_sampling_mode)
        .def("is_iq_swapped", &fe_connection_t::is_iq_swapped)
        .def("is_i_inverted", &fe_connection_t::is_i_inverted)
        .def("is_q_inverted", &fe_connection_t::is_q_inverted)
        .def("get_if_freq", &fe_connection_t::get_if_freq)
        .def("set_if_freq", &fe_connection_t::set_if_freq);
}

#endif /* INCLUDED_UHD_USRP_FE_CONNECTION_PYTHON_HPP */

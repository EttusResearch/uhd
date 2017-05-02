//
// Copyright 2017 Ettus Research
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

#ifndef INCLUDED_UHD_USRP_FE_CONNECTION_PYTHON_HPP
#define INCLUDED_UHD_USRP_FE_CONNECTION_PYTHON_HPP

#include <uhd/usrp/fe_connection.hpp>

void export_fe_connection()
{
    using fe_connection_t = uhd::usrp::fe_connection_t;
    using sampling_t      = fe_connection_t::sampling_t;

    bp::enum_<sampling_t>("sampling")
        .value("QUADRATURE", sampling_t::QUADRATURE)
        .value("HETERODYNE", sampling_t::HETERODYNE)
        .value("REAL"      , sampling_t::REAL      )
        ;

    bp::class_<fe_connection_t>
        ("fe_connection", bp::init<sampling_t, bool, bool, bool, double>())

        // Constructors
        .def(bp::init<const std::string&, double>())

        // Methods
        .def("get_sampling_mode", &fe_connection_t::get_sampling_mode)
        .def("is_iq_swapped"    , &fe_connection_t::is_iq_swapped    )
        .def("is_i_inverted"    , &fe_connection_t::is_i_inverted    )
        .def("is_q_inverted"    , &fe_connection_t::is_q_inverted    )
        .def("get_if_freq"      , &fe_connection_t::get_if_freq      )
        .def("set_if_freq"      , &fe_connection_t::set_if_freq      )
        ;
}

#endif /* INCLUDED_UHD_USRP_FE_CONNECTION_PYTHON_HPP */

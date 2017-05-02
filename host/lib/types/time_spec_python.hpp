//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TIME_SPEC_PYTHON_HPP
#define INCLUDED_UHD_TIME_SPEC_PYTHON_HPP

#include <uhd/types/time_spec.hpp>

void export_time_spec()
{
    using time_spec_t = uhd::time_spec_t;

    bp::class_<time_spec_t>("time_spec", bp::init<double>())

        // Methods
        .def("from_ticks"     , &time_spec_t::from_ticks     )
        .staticmethod("from_ticks"                           )

        .def("get_tick_count" , &time_spec_t::get_tick_count )
        .def("to_ticks"       , &time_spec_t::to_ticks       )
        .def("get_real_secs"  , &time_spec_t::get_real_secs  )
        .def("get_frac_secs"  , &time_spec_t::get_frac_secs  )

        .def(bp::self += time_spec_t())
        .def(bp::self += double())
        .def(bp::self + double())
        .def(bp::self + time_spec_t())
        .def(bp::self -= time_spec_t())
        ;
}

#endif /* INCLUDED_UHD_TIME_SPEC_PYTHON_HPP */

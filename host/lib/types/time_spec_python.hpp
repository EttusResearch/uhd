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

#ifndef INCLUDED_UHD_TIME_SPEC_PYTHON_HPP
#define INCLUDED_UHD_TIME_SPEC_PYTHON_HPP

#include <uhd/types/time_spec.hpp>

void export_time_spec()
{
    using time_spec_t = uhd::time_spec_t;

    bp::class_<time_spec_t>("time_spec", bp::init<double>())

        // Methods
        .def("get_system_time", &time_spec_t::get_system_time)
        .staticmethod("get_system_time"                      )

        .def("from_ticks"     , &time_spec_t::from_ticks     )
        .staticmethod("from_ticks"                           )

        .def("get_tick_count" , &time_spec_t::get_tick_count )
        .def("to_ticks"       , &time_spec_t::to_ticks       )
        .def("get_real_secs"  , &time_spec_t::get_real_secs  )
        .def("get_frac_secs"  , &time_spec_t::get_frac_secs  )

        .def(bp::self += time_spec_t())
        .def(bp::self -= time_spec_t())
        ;
}

#endif /* INCLUDED_UHD_TIME_SPEC_PYTHON_HPP */

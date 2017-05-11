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

#pragma once

#include "lockable.hpp"
#include "regs_iface.hpp"

void export_types() {
    LIBMPM_BOOST_PREAMBLE("types")
    using namespace mpm::types;
    bp::class_<lockable, boost::noncopyable, std::shared_ptr<lockable> >("lockable", bp::no_init)
        .def("lock", &lockable::lock)
        .def("unlock", &lockable::unlock)
    ;

    bp::class_<regs_iface, boost::noncopyable, std::shared_ptr<regs_iface> >("regs_iface", bp::no_init)
        .def("peek8", &regs_iface::peek8)
        .def("poke8", &regs_iface::poke8)
        .def("peek16", &regs_iface::peek16)
        .def("poke16", &regs_iface::poke16)
   ;
}


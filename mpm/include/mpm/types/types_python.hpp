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
#include "log_buf.hpp"
#include "mmap_regs_iface.hpp"

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

    bp::class_<log_buf, boost::noncopyable, std::shared_ptr<log_buf> >("log_buf", bp::no_init)
        .def("make_singleton", &log_buf::make_singleton)
        .staticmethod("make_singleton")
        .def("set_notify_callback", +[](log_buf& self,
                                        boost::python::object object) {
              self.set_notify_callback(object);
        })
        .def("pop", +[](log_buf& self){
            auto log_msg = self.pop();
            return bp::make_tuple(
                static_cast<int>(std::get<0>(log_msg)),
                std::get<1>(log_msg),
                std::get<2>(log_msg)
            );
        })
    ;

    bp::class_<mmap_regs_iface, boost::noncopyable, std::shared_ptr<mmap_regs_iface>>("mmap_regs_iface", bp::init<std::string, size_t, size_t, bool, bool>())
        .def("open", &mmap_regs_iface::open)
        .def("close", &mmap_regs_iface::close)
        .def("peek32", &mmap_regs_iface::peek32)
        .def("poke32", &mmap_regs_iface::poke32)
    ;
}


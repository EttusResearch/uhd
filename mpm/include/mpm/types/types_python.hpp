//
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "lockable.hpp"
#include "log_buf.hpp"
#include "mmap_regs_iface.hpp"
#include "regs_iface.hpp"

void export_types(py::module& top_module)
{
    using namespace mpm::types;
    auto m = top_module.def_submodule("types");

    py::class_<lockable, std::shared_ptr<lockable>>(m, "lockable")
        .def("lock", &lockable::lock)
        .def("unlock", &lockable::unlock);

    py::class_<regs_iface, std::shared_ptr<regs_iface>>(m, "regs_iface")
        .def("peek8", &regs_iface::peek8)
        .def("poke8", &regs_iface::poke8)
        .def("peek16", &regs_iface::peek16)
        .def("poke16", &regs_iface::poke16)
        .def("peek32", &regs_iface::peek32)
        .def("poke32", &regs_iface::poke32);

    py::class_<log_buf, std::shared_ptr<log_buf>>(m, "log_buf")
        .def_static("make_singleton", &log_buf::make_singleton)
        .def("set_notify_callback",
            +[](log_buf& self, py::object object) { self.set_notify_callback(object); })
        .def("pop", [](log_buf& self) {
            auto log_msg = self.pop();
            return py::make_tuple(static_cast<int>(std::get<0>(log_msg)),
                std::get<1>(log_msg),
                std::get<2>(log_msg));
        });

    py::class_<mmap_regs_iface, std::shared_ptr<mmap_regs_iface>>(m, "mmap_regs_iface")
        .def(py::init<std::string, size_t, size_t, bool, bool>())
        .def("open", &mmap_regs_iface::open)
        .def("close", &mmap_regs_iface::close)
        .def("peek32", &mmap_regs_iface::peek32)
        .def("poke32", &mmap_regs_iface::poke32);
}

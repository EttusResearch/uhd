//
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "spi_iface.hpp"
#include "spi_regs_iface.hpp"

void export_spi(py::module& top_module)
{
    auto m = top_module.def_submodule("spi");

    m.def("make_spidev_regs_iface", &mpm::spi::make_spidev_regs_iface);
    m.def("make_spidev", &mpm::spi::spi_iface::make_spidev);

    py::class_<mpm::spi::spi_iface, std::shared_ptr<mpm::spi::spi_iface>>(m, "spi_iface")
        .def("transfer24_8", &mpm::spi::spi_iface::transfer24_8)
        .def("transfer64_40", &mpm::spi::spi_iface::transfer64_40);
}

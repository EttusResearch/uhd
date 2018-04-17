//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "spi_regs_iface.hpp"
#include "spi_iface.hpp"

void export_spi() {
    LIBMPM_BOOST_PREAMBLE("spi")

    bp::def("make_spidev_regs_iface", &mpm::spi::make_spidev_regs_iface);
    bp::def("make_spidev", &mpm::spi::spi_iface::make_spidev);

    bp::class_<mpm::spi::spi_iface, boost::noncopyable, std::shared_ptr<mpm::spi::spi_iface> >("spi_iface", bp::no_init)
        .def("transfer24_8", &mpm::spi::spi_iface::transfer24_8)
    ;

}


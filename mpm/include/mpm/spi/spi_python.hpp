//
// Copyright 2017 Ettus Research, National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0
//

#pragma once

#include "spi_regs_iface.hpp"

void export_spi() {
    LIBMPM_BOOST_PREAMBLE("spi")

    bp::def("make_spidev_regs_iface", &mpm::spi::make_spidev_regs_iface);
}


//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "i2c_regs_iface.hpp"
#include "i2c_iface.hpp"

void export_i2c() {
    LIBMPM_BOOST_PREAMBLE("i2c")

    bp::def("make_i2cdev_regs_iface", &mpm::i2c::make_i2cdev_regs_iface);

}


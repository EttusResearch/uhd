//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "i2c_iface.hpp"
#include "i2c_regs_iface.hpp"

void export_i2c(py::module& top_module)
{
    auto m = top_module.def_submodule("i2c");

    m.def("make_i2cdev_regs_iface", &mpm::i2c::make_i2cdev_regs_iface);
}

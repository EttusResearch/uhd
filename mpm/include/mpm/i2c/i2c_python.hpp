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

    m.def("make_i2cdev", &mpm::i2c::i2c_iface::make_i2cdev);
    m.def("make_i2cdev_regs_iface", &mpm::i2c::make_i2cdev_regs_iface);

    py::class_<mpm::i2c::i2c_iface, std::shared_ptr<mpm::i2c::i2c_iface>>(m, "i2c_iface")
        .def("transfer", (std::vector<uint8_t> (mpm::i2c::i2c_iface::*)(std::vector<uint8_t>&, size_t, bool)) &mpm::i2c::i2c_iface::transfer, "Transfer i2c data");
}

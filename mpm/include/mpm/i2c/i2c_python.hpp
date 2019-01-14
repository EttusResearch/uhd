//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "i2c_iface.hpp"
#include "i2c_regs_iface.hpp"

void export_i2c()
{
    LIBMPM_BOOST_PREAMBLE("i2c")

    bp::def("make_i2cdev_regs_iface", &mpm::i2c::make_i2cdev_regs_iface);
    /*
        bp::def("make_i2cdev", &mpm::i2c::i2c_iface::make_i2cdev);

        int (mpm::i2c::i2c_iface::*transfer_vec)(std::vector<uint8_t>*,
                                                 std::vector<uint8_t>*) =
            &mpm::i2c::i2c_iface::transfer;

        bp::class_<mpm::i2c::i2c_iface, boost::noncopyable,
                   std::shared_ptr<mpm::i2c::i2c_iface> >("i2c_iface", bp::no_init)
            .def("transfer", transfer_vec)
        ;
    */
}

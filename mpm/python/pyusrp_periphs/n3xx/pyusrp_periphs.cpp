//
// Copyright 2017 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
namespace py = pybind11;
#define LIBMPM_PYTHON

#include <mpm/ad937x/ad937x_ctrl.hpp>
#include <mpm/dboards/magnesium_manager.hpp>
#include <mpm/i2c/i2c_python.hpp>
#include <mpm/spi/spi_python.hpp>
#include <mpm/types/types_python.hpp>

PYBIND11_MODULE(libpyusrp_periphs, m)
{
    export_types(m);
    export_spi(m);
    export_i2c(m);
    export_mykonos(m);
    export_magnesium(m);
}

//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <pybind11/pybind11.h>
namespace py = pybind11;
#define LIBMPM_PYTHON

#include <mpm/ad9361/ad9361_ctrl.hpp>
#include <mpm/dboards/e31x_db_manager.hpp>
#include <mpm/spi/spi_python.hpp>
#include <mpm/types/types_python.hpp>

PYBIND11_MODULE(libpyusrp_periphs, m)
{
    export_types(m);
    export_spi(m);
    export_catalina(m);
    export_e31x_db(m);
}

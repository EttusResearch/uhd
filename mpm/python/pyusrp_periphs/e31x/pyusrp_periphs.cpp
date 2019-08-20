//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <pybind11/pybind11.h>
namespace py = pybind11;
#define LIBMPM_PYTHON

// Allow boost::shared_ptr<T> to be a holder class of an object (PyBind11
// supports std::shared_ptr and std::unique_ptr out of the box)
#include <boost/shared_ptr.hpp>
PYBIND11_DECLARE_HOLDER_TYPE(T, boost::shared_ptr<T>);

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

//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include "device_python.hpp"
#include <uhd/device.hpp>

void export_device(py::module& m)
{
    m.def("find", [](const uhd::device_addr_t& hint) { return uhd::device::find(hint); });
}

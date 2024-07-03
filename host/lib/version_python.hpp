//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_VERSION_PYTHON_HPP
#define INCLUDED_UHD_VERSION_PYTHON_HPP

#include <uhd/version.hpp>

void export_version(py::module& m)
{
    m.def("get_version_string", &uhd::get_version_string);
    m.def("get_abi_string", &uhd::get_abi_string);
    m.def("get_component", &uhd::get_component);
}

#endif /* INCLUDED_UHD_VERSION_PYTHON_HPP */

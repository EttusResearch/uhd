//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_PATHS_PYTHON_HPP
#define INCLUDED_UHD_PATHS_PYTHON_HPP

#include <uhd/utils/paths.hpp>

void export_paths(py::module& m)
{
    m.def("get_tmp_path", &uhd::get_tmp_path);
    m.def("get_lib_path", &uhd::get_lib_path);
    m.def("get_pkg_path", &uhd::get_pkg_path);
    m.def("get_cal_data_path", &uhd::get_cal_data_path);
    m.def("get_images_dir", &uhd::get_images_dir);
    m.def("find_image_path", &uhd::find_image_path);
    m.def("find_utility", &uhd::find_utility);
    m.def("print_utility_error", &uhd::print_utility_error);
}

#endif /* INCLUDED_UHD_PATHS_PYTHON_HPP */

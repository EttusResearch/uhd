#
# Copyright 2010-2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
#
# Note: These variables are in a separate file so other projects can check these
# versions, but they are not prefixed with UHD_ so be aware of that.

set(UHD_CMAKE_MIN_VERSION           "3.12"     )
set(UHD_GCC_MIN_VERSION             "7.3.0"   )
set(UHD_CLANG_MIN_VERSION           "6.0.0"   )
set(UHD_APPLECLANG_MIN_VERSION      "800"     )
# Make sure to update the next two in unison:
set(UHD_MSVC_MIN_VERSION            "1910"    )
set(UHD_MSVC_MIN_VERSION_READABLE   "15.0"    )
# This Python version gets used for Python API (if requested) as well as
# all the build-time Python scripts
set(UHD_PYTHON_MIN_VERSION          "3.7"     )
# Other deps
set(UHD_SETUPTOOLS_MIN_VERSION      "40.0"    )
set(UHD_BOOST_MIN_VERSION           "1.71"    )
set(UHD_NUMPY_MIN_VERSION           "1.11"    )
set(UHD_RUAMEL_YAML_MIN_VERSION     "0.15"    )
set(UHD_PY_MAKO_MIN_VERSION         "0.4.2"   )
set(UHD_PY_REQUESTS_MIN_VERSION     "2.0"     )
set(UHD_PYBIND11_MIN_VERSION        "2.7"     )

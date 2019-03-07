#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

if(ENABLE_MPMD)
    add_subdirectory(rpclib)
endif()
if(ENABLE_PYTHON_API)
    add_subdirectory(pybind11)
endif()

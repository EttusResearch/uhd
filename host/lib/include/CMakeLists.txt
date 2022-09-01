#
# Copyright 2017 Ettus Research (National Instruments Corp.)
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/uhdlib/usrp/common/rpc.py
    ${CMAKE_CURRENT_BINARY_DIR}/uhdlib/usrp/common/rpc.hpp
)

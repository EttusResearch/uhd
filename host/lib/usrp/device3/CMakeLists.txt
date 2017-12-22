#
# Copyright 2014 Ettus Research LLC
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/device3_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/device3_io_impl.cpp
)

#
# Copyright 2011-2015 Ettus Research LLC
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/multi_usrp_clock.cpp
)

IF(ENABLE_C_API)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/usrp_clock_c.cpp
    )
ENDIF(ENABLE_C_API)

INCLUDE_SUBDIRECTORY(octoclock)

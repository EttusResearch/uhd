#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/adc_self_calibration.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/x400_radio_control.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/x400_rfdc_control.cpp
)

if(ENABLE_DPDK)
    include_directories(${DPDK_INCLUDE_DIRS})
    add_definitions(-DHAVE_DPDK)
endif(ENABLE_DPDK)

#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
include_directories(${UHD_SOURCE_DIR}/lib/deps/flatbuffers/include)
LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/database.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/iq_cal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pwr_cal.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dsa_cal.cpp
)


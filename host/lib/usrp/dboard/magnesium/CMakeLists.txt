#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

# This file is conditionally included if ENABLE_MPMD and ENABLE_N300 are
# set to true.

LIST(APPEND MAGNESIUM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium_radio_ctrl_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium_radio_ctrl_init.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium_radio_ctrl_cpld.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium_radio_ctrl_gain.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium_ad9371_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium_bands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium_cpld_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium_gain_table.cpp
)
LIBUHD_APPEND_SOURCES(${MAGNESIUM_SOURCES})


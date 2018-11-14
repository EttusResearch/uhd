#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

if(ENABLE_MPMD)
    list(APPEND RHODIUM_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/rhodium_radio_ctrl_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/rhodium_radio_ctrl_init.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/rhodium_radio_ctrl_cpld.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/rhodium_radio_ctrl_lo.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/rhodium_bands.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/rhodium_cpld_ctrl.cpp
    )
    LIBUHD_APPEND_SOURCES(${RHODIUM_SOURCES})
endif(ENABLE_MPMD)


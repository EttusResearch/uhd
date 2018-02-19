#
# Copyright 2010-2011,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Conditionally configure the USRP1 support
########################################################################
IF(ENABLE_USRP1)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/codec_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dboard_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/io_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mb_eeprom.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/soft_time_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usrp1_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usrp1_impl.cpp
    )
ENDIF(ENABLE_USRP1)

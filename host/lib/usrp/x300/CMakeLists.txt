#
# Copyright 2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Conditionally configure the X300 support
########################################################################
IF(ENABLE_X300)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_radio_ctrl_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_fw_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_fw_uart.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_adc_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_dac_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_io_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_dboard_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_clock_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_image_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_mb_eeprom_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_mb_eeprom.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cdecode.c
    )
ENDIF(ENABLE_X300)

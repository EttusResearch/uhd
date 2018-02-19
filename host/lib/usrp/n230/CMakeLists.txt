#
# Copyright 2013 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Conditionally configure the N230 support
########################################################################
IF(ENABLE_N230)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_cores.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_resource_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_eeprom_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_stream_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_clk_pps_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_frontend_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_uart.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_image_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_fw_ctrl_iface.cpp
   )
ENDIF(ENABLE_N230)

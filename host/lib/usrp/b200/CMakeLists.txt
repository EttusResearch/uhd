#
# Copyright 2012-2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Conditionally configure the B200 support
########################################################################
IF(ENABLE_B200)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_image_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_io_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_uart.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_cores.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_mb_eeprom.cpp
    )
ENDIF(ENABLE_B200)

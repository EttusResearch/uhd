#
# Copyright 2011-2012,2014-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Conditionally configure the USRP2 support
########################################################################
IF(ENABLE_USRP2)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/clock_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/codec_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dboard_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/io_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mb_eeprom.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usrp2_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usrp2_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usrp2_fifo_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n200_image_loader.cpp
    )
ENDIF(ENABLE_USRP2)

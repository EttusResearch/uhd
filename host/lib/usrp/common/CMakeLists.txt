#
# Copyright 2011-2013,2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################
IF(ENABLE_USB)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/fx2_ctrl.cpp
    )
ENDIF(ENABLE_USB)

IF(ENABLE_E300 OR ENABLE_B200 OR ENABLE_N230)
    INCLUDE_DIRECTORIES("${CMAKE_CURRENT_SOURCE_DIR}/ad9361_driver")
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/ad9361_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ad936x_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ad9361_driver/ad9361_device.cpp
    )
ENDIF(ENABLE_E300 OR ENABLE_B200 OR ENABLE_N230)

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/adf4001_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adf435x.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adf535x.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lmx2592.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apply_corrections.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/validate_subdev_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/recv_packet_demuxer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fifo_ctrl_excelsior.cpp
)

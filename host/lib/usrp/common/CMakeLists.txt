#
# Copyright 2011-2013,2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
if(ENABLE_USB)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/fx2_ctrl.cpp
    )
endif(ENABLE_USB)

if(ENABLE_E300 OR ENABLE_B200 OR ENABLE_E320)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ad9361_driver")
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/ad9361_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ad936x_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ad9361_driver/ad9361_device.cpp
    )
endif(ENABLE_E300 OR ENABLE_B200 OR ENABLE_E320)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/adf4001_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adf435x.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adf535x.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lmx2592.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lmx2572.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apply_corrections.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/validate_subdev_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/recv_packet_demuxer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/io_service_mgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/io_service_args.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pwr_cal_mgr.cpp
)

if(ENABLE_DPDK)
    set_property(
        SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/io_service_mgr.cpp
        PROPERTY COMPILE_DEFINITIONS HAVE_DPDK
    )
endif(ENABLE_DPDK)

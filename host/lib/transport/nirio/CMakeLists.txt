#
# Copyright 2013-2014 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Include subdirectories (different than add)
########################################################################
INCLUDE_SUBDIRECTORY(lvbitx)
INCLUDE_SUBDIRECTORY(rpc)

########################################################################
# Append to the list of sources for lib uhd
########################################################################

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/nifpga_lvbitx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/niusrprio_session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/niriok_proxy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/niriok_proxy_impl_v1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/niriok_proxy_impl_v2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nirio_resource_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/status.cpp
)

IF(WIN32)
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/nirio_driver_iface_win.cpp)
ELSEIF(${CMAKE_SYSTEM_NAME} STREQUAL "Linux") #Built-in variable encompasses all UNIX-like systems
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/nirio_driver_iface_linux.cpp)
ELSE()
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/nirio_driver_iface_unsupported.cpp)
ENDIF(WIN32)

#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

macro(CATALINA_APPEND_SOURCES)
    set(catalina_sources ${catalina_sources})
    list(APPEND catalina_sources ${ARGV})
    set(catalina_sources ${catalina_sources} PARENT_SCOPE)
endmacro(CATALINA_APPEND_SOURCES)

set (CMAKE_CATALINA_SOURCE_DIR
    ${UHD_HOST_ROOT}/lib/usrp/common
)

########################################################################
# Setup UHD_VERSION and UHD_COMPONENT for version.cpp
########################################################################
configure_file(
    ${UHD_HOST_ROOT}/include/uhd/version.hpp.in
    ${CMAKE_BINARY_DIR}/include/uhd/version.hpp
)
configure_file(
    ${UHD_HOST_ROOT}/lib/version.cpp
    ${CMAKE_BINARY_DIR}/lib/version.cpp
)

# TODO: Do something better than listing all the files individually
set(catalina_sources
    ${CMAKE_CATALINA_SOURCE_DIR}/ad9361_ctrl.cpp
    ${CMAKE_CATALINA_SOURCE_DIR}/ad936x_manager.cpp
    ${CMAKE_CATALINA_SOURCE_DIR}/ad9361_driver/ad9361_device.cpp
    ${UHD_HOST_ROOT}/lib/types/filters.cpp
    ${UHD_HOST_ROOT}/lib/types/ranges.cpp
    ${UHD_HOST_ROOT}/lib/types/sensors.cpp
    ${UHD_HOST_ROOT}/lib/types/serial.cpp
    ${UHD_HOST_ROOT}/lib/property_tree.cpp
    ${UHD_HOST_ROOT}/lib/utils/log.cpp
    ${CMAKE_BINARY_DIR}/lib/version.cpp
)

USRP_PERIPHS_ADD_OBJECT(catalina ${catalina_sources})

target_include_directories(catalina PUBLIC
                           $<BUILD_INTERFACE:${UHD_HOST_ROOT}/lib/include>)
target_include_directories(catalina PUBLIC
                           $<BUILD_INTERFACE:${UHD_HOST_ROOT}/lib/usrp/common/ad9361_driver>)

target_include_directories(catalina PUBLIC
                           $<BUILD_INTERFACE:${UHD_HOST_ROOT}/lib/include>)
target_include_directories(dboards PUBLIC
                           $<BUILD_INTERFACE:${UHD_HOST_ROOT}/lib/usrp/common/ad9361_driver>)

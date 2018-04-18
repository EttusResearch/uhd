#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

MACRO(CATALINA_APPEND_SOURCES)
    SET(catalina_sources ${catalina_sources})
    LIST(APPEND catalina_sources ${ARGV})
    SET(catalina_sources ${catalina_sources} PARENT_SCOPE)
ENDMACRO(CATALINA_APPEND_SOURCES)

SET (CMAKE_CATALINA_SOURCE_DIR
    ${UHD_HOST_ROOT}/lib/usrp/common
)

CONFIGURE_FILE(
    ${UHD_HOST_ROOT}/include/uhd/version.hpp.in
    ${CMAKE_BINARY_DIR}/include/uhd/version.hpp
)

# TODO: Do something better than listing all the files individually
SET(catalina_sources
    ${CMAKE_CATALINA_SOURCE_DIR}/ad9361_ctrl.cpp
    ${CMAKE_CATALINA_SOURCE_DIR}/ad936x_manager.cpp
    ${CMAKE_CATALINA_SOURCE_DIR}/ad9361_driver/ad9361_device.cpp
    ${UHD_HOST_ROOT}/lib/types/filters.cpp
    ${UHD_HOST_ROOT}/lib/types/ranges.cpp
    ${UHD_HOST_ROOT}/lib/types/sensors.cpp
    ${UHD_HOST_ROOT}/lib/types/serial.cpp
    ${UHD_HOST_ROOT}/lib/property_tree.cpp
    ${UHD_HOST_ROOT}/lib/utils/log.cpp
    ${UHD_HOST_ROOT}/lib/version.cpp
)

USRP_PERIPHS_ADD_OBJECT(catalina ${catalina_sources})

TARGET_INCLUDE_DIRECTORIES(catalina PUBLIC
                           $<BUILD_INTERFACE:${UHD_HOST_ROOT}/lib/include>)
TARGET_INCLUDE_DIRECTORIES(catalina PUBLIC
                           $<BUILD_INTERFACE:${UHD_HOST_ROOT}/lib/usrp/common/ad9361_driver>)

TARGET_INCLUDE_DIRECTORIES(catalina PUBLIC
                           $<BUILD_INTERFACE:${UHD_HOST_ROOT}/lib/include>)
TARGET_INCLUDE_DIRECTORIES(dboards PUBLIC
                           $<BUILD_INTERFACE:${UHD_HOST_ROOT}/lib/usrp/common/ad9361_driver>)

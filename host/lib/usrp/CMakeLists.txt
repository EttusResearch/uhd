#
# Copyright 2010-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/dboard_base.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dboard_eeprom.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dboard_id.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dboard_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dboard_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gps_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/multi_usrp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/subdev_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fe_connection.cpp
)

IF(ENABLE_C_API)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/dboard_eeprom_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mboard_eeprom_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/subdev_spec_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usrp_c.cpp
    )
ENDIF(ENABLE_C_API)

IF(ENABLE_GPSD)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/gpsd_iface.cpp
    )
    LIBUHD_APPEND_LIBS(${LIBGPS_LIBRARY})
ENDIF(ENABLE_GPSD)

INCLUDE_SUBDIRECTORY(cores)
INCLUDE_SUBDIRECTORY(dboard)
INCLUDE_SUBDIRECTORY(common)
INCLUDE_SUBDIRECTORY(device3)
INCLUDE_SUBDIRECTORY(mpmd)
INCLUDE_SUBDIRECTORY(usrp1)
INCLUDE_SUBDIRECTORY(usrp2)
INCLUDE_SUBDIRECTORY(b100)
INCLUDE_SUBDIRECTORY(e300)
INCLUDE_SUBDIRECTORY(x300)
INCLUDE_SUBDIRECTORY(b200)
INCLUDE_SUBDIRECTORY(n230)

#
# Copyright 2011-2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################
LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/device_addr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/mac_addr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/metadata.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ranges.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sensors.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/serial.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/time_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tune.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/wb_iface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/filters.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/byte_vector.cpp
)

IF(ENABLE_C_API)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/metadata_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ranges_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/sensors_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/string_vector_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tune_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usrp_info_c.cpp
    )
ENDIF()

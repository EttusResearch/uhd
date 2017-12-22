#
# Copyright 2016 Ettus Research
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/expert_container.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/expert_factory.cpp
)

# Verbose Debug output for send/recv
SET( UHD_EXPERT_LOGGING OFF CACHE BOOL "Enable verbose logging for experts" )
OPTION( UHD_EXPERT_LOGGING "Enable verbose logging for experts" "" )
IF(UHD_EXPERT_LOGGING)
    MESSAGE(STATUS "Enabling verbose logging for experts")
    ADD_DEFINITIONS(-DUHD_EXPERT_LOGGING)
ENDIF()

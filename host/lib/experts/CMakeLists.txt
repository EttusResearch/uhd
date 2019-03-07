#
# Copyright 2016 Ettus Research
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/expert_container.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/expert_factory.cpp
)

# Verbose Debug output for send/recv
set( UHD_EXPERT_LOGGING OFF CACHE BOOL "Enable verbose logging for experts" )
option( UHD_EXPERT_LOGGING "Enable verbose logging for experts" "" )
if(UHD_EXPERT_LOGGING)
    message(STATUS "Enabling verbose logging for experts")
    add_definitions(-DUHD_EXPERT_LOGGING)
endif()

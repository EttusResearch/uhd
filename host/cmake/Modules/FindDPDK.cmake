#
# Copyright 2018 Ettus Research, a National Instruments Company
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# - Find DPDK
# Find the DPDK includes and client library
# This module defines
#  DPDK_INCLUDE_DIRS, where to find rte_config.h and rte_version.h
#  DPDK_LIBRARIES, the libraries needed by a DPDK user
#  DPDK_FOUND, If false, do not try to use DPDK.
# also defined, but not for general use are
#  DPDK_LIBRARY, where to find the DPDK library.

include(FindPackageHandleStandardArgs)

function(DPDK_READ_VERSION DPDK_VERSION DPDK_INCLUDE_DIRS)
    if(NOT DPDK_INCLUDE_DIRS)
        return()
    endif()

    file(READ "${DPDK_INCLUDE_DIRS}/rte_version.h"
        DPDK_VERSION_STR
    )
    string(REGEX MATCH "#define RTE_VER_YEAR ([0-9]+)" _ ${DPDK_VERSION_STR})
    set(DPDK_VERSION_MAJOR ${CMAKE_MATCH_1})

    string(REGEX MATCH "#define RTE_VER_MONTH ([0-9]+)" _ ${DPDK_VERSION_STR})
    set(DPDK_VERSION_MINOR ${CMAKE_MATCH_1})

    set(DPDK_VERSION "${DPDK_VERSION_MAJOR}.${DPDK_VERSION_MINOR}" PARENT_SCOPE)
endfunction()

if(DPDK_LIBRARIES AND DPDK_INCLUDE_DIRS)
    set(DPDK_USER_PROVIDED ON)
else()
    set(DPDK_USER_PROVIDED OFF)
endif()

find_package(PkgConfig)
PKG_CHECK_MODULES(PC_DPDK QUIET libdpdk>=18.11)

find_path (DPDK_VERSION_INCLUDE_DIR rte_version.h
    HINTS ${PC_DPDK_INCLUDE_DIRS}
    PATHS ENV RTE_INCLUDE
    PATH_SUFFIXES dpdk
)
find_path (DPDK_CONFIG_INCLUDE_DIR rte_config.h
    HINTS ${PC_DPDK_INCLUDE_DIRS}
    PATHS ENV RTE_INCLUDE
    PATH_SUFFIXES dpdk
)

# Check for linker script that pulls in the APIs
find_library(DPDK_LIBRARY
    dpdk
    HINTS ${PC_DPDK_LIBDIR}
    PATHS $ENV{RTE_SDK_DIR}/$ENV{RTE_TARGET}/lib
)

if(NOT DPDK_USER_PROVIDED)
    set(DPDK_INCLUDE_DIRS ${PC_DPDK_INCLUDE_DIRS})
    list(APPEND DPDK_INCLUDE_DIRS ${DPDK_VERSION_INCLUDE_DIR})
    list(APPEND DPDK_INCLUDE_DIRS ${DPDK_CONFIG_INCLUDE_DIR})
    list(REMOVE_DUPLICATES DPDK_INCLUDE_DIRS)
endif()

if(DPDK_USER_PROVIDED)
    DPDK_READ_VERSION(DPDK_VERSION ${DPDK_INCLUDE_DIRS})
else()
    set(DPDK_LIBRARIES ${PC_DPDK_LIBRARIES})

    if(DPDK_LIBRARY)
        list(APPEND DPDK_LIBRARIES ${DPDK_LIBRARY})
        list(REMOVE_DUPLICATES DPDK_LIBRARIES)
    endif()

    if(PC_DPDK_FOUND)
        set(DPDK_VERSION ${PC_DPDK_VERSION})
    else()
        DPDK_READ_VERSION(DPDK_VERSION ${DPDK_INCLUDE_DIRS})
    endif(PC_DPDK_FOUND)
endif()

find_package_handle_standard_args(DPDK
    REQUIRED_VARS DPDK_INCLUDE_DIRS DPDK_LIBRARIES
    VERSION_VAR DPDK_VERSION
)

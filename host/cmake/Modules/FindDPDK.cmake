#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# - Find DPDK
# Find the DPDK includes and client library
# This module defines
#  DPDK_INCLUDE_DIR, where to find rte_config.h
#  DPDK_LIBRARIES, the libraries needed by a DPDK user
#  DPDK_FOUND, If false, do not try to use DPDK.
# also defined, but not for general use are
#  DPDK_LIBRARY, where to find the DPDK library.

include(FindPackageHandleStandardArgs)

find_path ( DPDK_INCLUDE_CONFIG_DIR rte_config.h
    PATHS ENV RTE_INCLUDE
    PATH_SUFFIXES dpdk
)

find_path ( DPDK_INCLUDE_ETHDEV_DIR rte_ethdev.h
    PATHS ENV RTE_INCLUDE
    PATH_SUFFIXES dpdk
)

find_path ( DPDK_INCLUDE_VERSION_DIR rte_version.h
    PATHS ENV RTE_INCLUDE
    PATH_SUFFIXES dpdk
)

set(DPDK_INCLUDE_DIR ${DPDK_INCLUDE_CONFIG_DIR} ${DPDK_INCLUDE_ETHDEV_DIR})
list(REMOVE_DUPLICATES DPDK_INCLUDE_DIR)

find_library(DPDK_LIBRARY
    PATHS $ENV{RTE_SDK_DIR}/$ENV{RTE_TARGET}/lib
)

list(APPEND DPDK_LIBRARIES dpdk)

if(DPDK_INCLUDE_VERSION_DIR)
    file(READ "${DPDK_INCLUDE_VERSION_DIR}/rte_version.h"
        DPDK_VERSION_STR
    )
    
    string(REGEX MATCH "#define RTE_VER_YEAR ([0-9]+)" _ ${DPDK_VERSION_STR})
    set(DPDK_VERSION_MAJOR ${CMAKE_MATCH_1})
    
    string(REGEX MATCH "#define RTE_VER_MONTH ([0-9]+)" _ ${DPDK_VERSION_STR})
    set(DPDK_VERSION_MINOR ${CMAKE_MATCH_1})
endif(DPDK_INCLUDE_VERSION_DIR)


set(DPDK_VERSION "${DPDK_VERSION_MAJOR}.${DPDK_VERSION_MINOR}")

find_package_handle_standard_args(DPDK
    REQUIRED_VARS DPDK_INCLUDE_DIR DPDK_LIBRARIES
    VERSION_VAR DPDK_VERSION
)

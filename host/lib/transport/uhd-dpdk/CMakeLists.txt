#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Add the subdirectories
########################################################################
if(ENABLE_DPDK)
    if(NOT DEFINED UHD_DPDK_CFLAGS)
        message(STATUS "")
        set(UHD_DPDK_CFLAGS "-march=native"
            CACHE STRING "CFLAGS to use when building uhd-dpdk sources")
        message(STATUS "DPDK: Using default UHD_DPDK_CFLAGS=" ${UHD_DPDK_CFLAGS})
    endif(NOT DEFINED UHD_DPDK_CFLAGS)

    include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/dpdk_common.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dpdk_io_service.cpp
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/dpdk_common.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dpdk_io_service.cpp
        PROPERTIES COMPILE_FLAGS "${UHD_DPDK_CFLAGS} -D_GNU_SOURCE"
    )
    include_directories(${DPDK_INCLUDE_DIRS})
    LIBUHD_APPEND_LIBS(${DPDK_LIBRARIES})
endif(ENABLE_DPDK)


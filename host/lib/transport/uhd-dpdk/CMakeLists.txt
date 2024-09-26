#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Add the subdirectories
########################################################################
if(ENABLE_DPDK)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR})

    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/dpdk_common.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dpdk_io_service.cpp
    )
endif(ENABLE_DPDK)


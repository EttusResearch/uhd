#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

if(ENABLE_MPMD)
    if(ENABLE_LIBERIO)
        message(STATUS "Compiling MPMD with liberio support...")
        add_definitions(-DHAVE_LIBERIO)
    endif(ENABLE_LIBERIO)

    if(ENABLE_DPDK)
        message(STATUS "Compiling MPMD with DPDK support...")
        add_definitions(-DHAVE_DPDK)
    endif(ENABLE_DPDK)

    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_find.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_image_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_mboard_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_prop_tree.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_xport.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_xport_mgr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_xport_ctrl_udp.cpp
    )

    if(ENABLE_LIBERIO)
        LIBUHD_APPEND_SOURCES(
            ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_xport_ctrl_liberio.cpp
        )
    endif(ENABLE_LIBERIO)

    if(ENABLE_DPDK)
        LIBUHD_APPEND_SOURCES(
            ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_xport_ctrl_dpdk_udp.cpp
        )
    endif(ENABLE_DPDK)

endif(ENABLE_MPMD)

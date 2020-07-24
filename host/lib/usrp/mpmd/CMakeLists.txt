#
# Copyright 2017 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

if(ENABLE_MPMD)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_find.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_image_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_mboard_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_mb_controller.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_mb_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_prop_tree.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_link_if_mgr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_link_if_ctrl_udp.cpp
    )

    if(ENABLE_SIM)
        LIBUHD_APPEND_SOURCES(
            ${CMAKE_CURRENT_SOURCE_DIR}/sim_find.cpp
        )
    endif(ENABLE_SIM)

    if(ENABLE_DPDK)
        include_directories(${DPDK_INCLUDE_DIRS})
        set_property(
            SOURCE
            ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_link_if_ctrl_udp.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_find.cpp
            PROPERTY COMPILE_DEFINITIONS HAVE_DPDK
        )
    endif(ENABLE_DPDK)

endif(ENABLE_MPMD)

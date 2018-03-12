#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

IF(ENABLE_MPMD)
    IF(ENABLE_LIBERIO)
        MESSAGE(STATUS "Compiling MPMD with liberio support...")
        ADD_DEFINITIONS(-DHAVE_LIBERIO)
    ENDIF(ENABLE_LIBERIO)

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

    IF(ENABLE_LIBERIO)
        LIBUHD_APPEND_SOURCES(
            ${CMAKE_CURRENT_SOURCE_DIR}/mpmd_xport_ctrl_liberio.cpp
        )
    ENDIF(ENABLE_LIBERIO)

ENDIF(ENABLE_MPMD)

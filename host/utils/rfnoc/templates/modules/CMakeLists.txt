#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
set(RFNOC_PKG_FILES ${RFNOC_PKG_FILES})
set(RFNOC_PKG_TOP_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_xb_sep_transport.v.mako
    ${CMAKE_CURRENT_SOURCE_DIR}/connect_io_ports.v.mako
    ${CMAKE_CURRENT_SOURCE_DIR}/ctrl_crossbar.v.mako
    ${CMAKE_CURRENT_SOURCE_DIR}/device_io_ports.v.mako
    ${CMAKE_CURRENT_SOURCE_DIR}/device_transport.v.mako
    ${CMAKE_CURRENT_SOURCE_DIR}/drive_unused_ports.v.mako
    ${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_block.v.mako
    ${CMAKE_CURRENT_SOURCE_DIR}/sep_xb_wires.v.mako
    ${CMAKE_CURRENT_SOURCE_DIR}/static_router.v.mako
    ${CMAKE_CURRENT_SOURCE_DIR}/stream_endpoints.v.mako
)
list(APPEND RFNOC_PKG_FILES ${RFNOC_PKG_TOP_FILES})
set(RFNOC_PKG_FILES ${RFNOC_PKG_FILES} PARENT_SCOPE)

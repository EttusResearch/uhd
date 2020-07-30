#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

set(USRP_MPM_FILES ${USRP_MPM_FILES})
set(USRP_MPM_SIMULATOR_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/sim_dboard.py
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware_presets.py
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_endpoint.py
    ${CMAKE_CURRENT_SOURCE_DIR}/noc_block_regs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_graph.py
    ${CMAKE_CURRENT_SOURCE_DIR}/stream_ep_regs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/sample_source.py
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_stream.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_common.py
    ${CMAKE_CURRENT_SOURCE_DIR}/stream_endpoint_node.py
    ${CMAKE_CURRENT_SOURCE_DIR}/config.py
)
list(APPEND USRP_MPM_FILES ${USRP_MPM_SIMULATOR_FILES})
set(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)

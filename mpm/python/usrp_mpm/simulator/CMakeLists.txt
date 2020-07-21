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
    ${CMAKE_CURRENT_SOURCE_DIR}/sim_dboard_catalina.py
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_sniffer.py
    ${CMAKE_CURRENT_SOURCE_DIR}/noc_block_regs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_graph.py
    ${CMAKE_CURRENT_SOURCE_DIR}/stream_ep_regs.py
)
list(APPEND USRP_MPM_FILES ${USRP_MPM_SIMULATOR_FILES})
set(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)

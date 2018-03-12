#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

SET(USRP_MPM_FILES ${USRP_MPM_FILES})
SET(USRP_MPM_XPORT_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/xportmgr_udp.py
    ${CMAKE_CURRENT_SOURCE_DIR}/xportmgr_liberio.py
)
LIST(APPEND USRP_MPM_FILES ${USRP_MPM_XPORT_FILES})
SET(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)


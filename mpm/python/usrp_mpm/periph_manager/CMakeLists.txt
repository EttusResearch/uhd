#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
set(USRP_MPM_FILES ${USRP_MPM_FILES})
set(USRP_MPM_PERIPHMGR_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py.in
    ${CMAKE_CURRENT_SOURCE_DIR}/base.py
    ${CMAKE_CURRENT_SOURCE_DIR}/common.py
    ${CMAKE_CURRENT_SOURCE_DIR}/n3xx.py
    ${CMAKE_CURRENT_SOURCE_DIR}/n3xx_periphs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/e320.py
    ${CMAKE_CURRENT_SOURCE_DIR}/e320_periphs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/e31x.py
    ${CMAKE_CURRENT_SOURCE_DIR}/e31x_periphs.py
    )
list(APPEND USRP_MPM_FILES ${USRP_MPM_PERIPHMGR_FILES})
set(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)

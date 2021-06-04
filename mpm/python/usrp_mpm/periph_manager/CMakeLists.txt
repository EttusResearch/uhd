#
# Copyright 2017-2019 Ettus Research, a National Instruments Company
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
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_periphs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_clk_aux.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_clk_mgr.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_sample_pll.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_reference_pll.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_update_cpld.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_gps_mgr.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_mb_cpld.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_rfdc_regs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_rfdc_ctrl.py
    ${CMAKE_CURRENT_SOURCE_DIR}/sim.py
    )
list(APPEND USRP_MPM_FILES ${USRP_MPM_PERIPHMGR_FILES})
set(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)

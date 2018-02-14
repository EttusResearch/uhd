#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################
SET(USRP_MPM_FILES ${USRP_MPM_FILES})
SET(USRP_MPM_DBMGR_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/base.py
    ${CMAKE_CURRENT_SOURCE_DIR}/eiscat.py
    ${CMAKE_CURRENT_SOURCE_DIR}/lmk_eiscat.py
    ${CMAKE_CURRENT_SOURCE_DIR}/lmk_mg.py
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium.py
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium_update_cpld.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mg_periphs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/test.py
    ${CMAKE_CURRENT_SOURCE_DIR}/unknown.py
)
LIST(APPEND USRP_MPM_FILES ${USRP_MPM_DBMGR_FILES})
SET(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)

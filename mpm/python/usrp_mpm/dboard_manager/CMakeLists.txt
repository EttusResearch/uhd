#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
set(USRP_MPM_FILES ${USRP_MPM_FILES})
set(USRP_MPM_DBMGR_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/base.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rhodium.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rh_init.py
    ${CMAKE_CURRENT_SOURCE_DIR}/rh_periphs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/lmk_rh.py
    ${CMAKE_CURRENT_SOURCE_DIR}/adc_rh.py
    ${CMAKE_CURRENT_SOURCE_DIR}/dac_rh.py
    ${CMAKE_CURRENT_SOURCE_DIR}/ad936x_db.py
    ${CMAKE_CURRENT_SOURCE_DIR}/neon.py
    ${CMAKE_CURRENT_SOURCE_DIR}/e31x_db.py
    ${CMAKE_CURRENT_SOURCE_DIR}/gain_rh.py
    ${CMAKE_CURRENT_SOURCE_DIR}/gaintables_rh.py
    ${CMAKE_CURRENT_SOURCE_DIR}/lmk_mg.py
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium.py
    ${CMAKE_CURRENT_SOURCE_DIR}/magnesium_update_cpld.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mg_init.py
    ${CMAKE_CURRENT_SOURCE_DIR}/mg_periphs.py
    ${CMAKE_CURRENT_SOURCE_DIR}/zbx.py
    ${CMAKE_CURRENT_SOURCE_DIR}/test.py
    ${CMAKE_CURRENT_SOURCE_DIR}/unknown.py
    ${CMAKE_CURRENT_SOURCE_DIR}/dboard_iface.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_db_iface.py
    ${CMAKE_CURRENT_SOURCE_DIR}/zbx_update_cpld.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_debug_db.py
    ${CMAKE_CURRENT_SOURCE_DIR}/x4xx_if_test_cca.py
)
list(APPEND USRP_MPM_FILES ${USRP_MPM_DBMGR_FILES})
set(USRP_MPM_FILES ${USRP_MPM_FILES} PARENT_SCOPE)

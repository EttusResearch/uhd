#
# Copyright 2010-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################
IF(ENABLE_X300 OR ENABLE_USRP1 OR ENABLE_USRP2 OR ENABLE_B100)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/db_basic_and_lf.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_rfx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_xcvr2450.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_sbx_common.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_sbx_version3.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_sbx_version4.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_cbx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_ubx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_wbx_common.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_wbx_version2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_wbx_version3.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_wbx_version4.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_wbx_simple.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_dbsrx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_unknown.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_tvrx.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_dbsrx2.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_tvrx2.cpp
    )
ENDIF(ENABLE_X300 OR ENABLE_USRP1 OR ENABLE_USRP2 OR ENABLE_B100)

IF(ENABLE_E300)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/db_e3x0.cpp
    )
ENDIF(ENABLE_E300)

IF(ENABLE_X300)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/twinrx/twinrx_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/twinrx/twinrx_experts.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/twinrx/twinrx_gain_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_twinrx.cpp
    )
ENDIF(ENABLE_X300)

IF(ENABLE_N300)
    INCLUDE_SUBDIRECTORY(magnesium)
ENDIF(ENABLE_N300)
IF(ENABLE_MPMD AND ENABLE_EISCAT)
    INCLUDE_SUBDIRECTORY(eiscat)
ENDIF(ENABLE_MPMD AND ENABLE_EISCAT)

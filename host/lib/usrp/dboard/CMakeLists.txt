#
# Copyright 2010-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
if(ENABLE_X300 OR ENABLE_USRP1 OR ENABLE_USRP2 OR ENABLE_B100)
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
endif(ENABLE_X300 OR ENABLE_USRP1 OR ENABLE_USRP2 OR ENABLE_B100)

if(ENABLE_X300)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/twinrx/twinrx_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/twinrx/twinrx_experts.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/twinrx/twinrx_gain_tables.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/db_twinrx.cpp
    )
endif(ENABLE_X300)

if(ENABLE_E300 OR ENABLE_E320)
    INCLUDE_SUBDIRECTORY(e3xx)
endif(ENABLE_E300 OR ENABLE_E320)
if(ENABLE_N300)
    INCLUDE_SUBDIRECTORY(magnesium)
endif(ENABLE_N300)
if(ENABLE_N320)
    INCLUDE_SUBDIRECTORY(rhodium)
endif(ENABLE_N320)
if(ENABLE_MPMD AND ENABLE_EISCAT)
    INCLUDE_SUBDIRECTORY(eiscat)
endif(ENABLE_MPMD AND ENABLE_EISCAT)

if(ENABLE_X400)
    INCLUDE_SUBDIRECTORY(zbx)
endif(ENABLE_X400)

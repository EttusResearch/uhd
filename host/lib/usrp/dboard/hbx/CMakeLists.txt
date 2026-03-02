#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

IF(ENABLE_X400)
        LIST(APPEND X410_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/hbx_dboard.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hbx_dboard_init.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hbx_cpld_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hbx_expert.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hbx_lo_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hbx_demod_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hbx_admv1320_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hbx_admv1420_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/hbx_lo_pd.cpp
    )
    LIBUHD_APPEND_SOURCES(${X410_SOURCES})
ENDIF(ENABLE_X400)


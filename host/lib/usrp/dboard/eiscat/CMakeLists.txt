#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

IF(ENABLE_MPMD AND ENABLE_EISCAT)
    LIST(APPEND EISCAT_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/eiscat_radio_ctrl_impl.cpp
    )
    LIBUHD_APPEND_SOURCES(${EISCAT_SOURCES})
ENDIF(ENABLE_MPMD AND ENABLE_EISCAT)


#
# Copyright 2017 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

# This file is conditionally included if ENABLE_MPMD and ENABLE_EISCAT are
# set to true.

LIST(APPEND EISCAT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/eiscat_radio_ctrl_impl.cpp
)
LIBUHD_APPEND_SOURCES(${EISCAT_SOURCES})


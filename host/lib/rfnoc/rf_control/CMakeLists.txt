#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/antenna.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gain_profile.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nameless_gain_mixin.cpp
)

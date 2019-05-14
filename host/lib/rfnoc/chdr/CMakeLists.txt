#
# Copyright 2014-2015,2017 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr_packet.cpp
)

#
# Copyright 2013 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Append to the list of sources for lib uhd
########################################################################

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/rpc_client.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/usrprio_rpc_client.cpp
)

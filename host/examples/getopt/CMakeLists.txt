#
# Copyright 2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# getopt library for C examples since MSVC does not include it
########################################################################
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIRECTORY})
ADD_LIBRARY(getopt STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/getopt.c
)

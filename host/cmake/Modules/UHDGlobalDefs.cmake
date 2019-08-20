#
# Copyright 2015,2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# This file sets up all the stuff for the config.h file

include(CheckCXXSymbolExists)

## Macros for the version number
if(UHD_VERSION_DEVEL)
    math(EXPR UHD_VERSION_ADDED "1000000 * ${UHD_VERSION_MAJOR} + 10000 * ${UHD_VERSION_API} + 100 * ${UHD_VERSION_ABI} + 99")
else()
    math(EXPR UHD_VERSION_ADDED "1000000 * ${UHD_VERSION_MAJOR} + 10000 * ${UHD_VERSION_API} + 100 * ${UHD_VERSION_ABI} + ${UHD_VERSION_PATCH}")
endif(UHD_VERSION_DEVEL)

## make sure the code knows about config.h
add_definitions(-DHAVE_CONFIG_H)

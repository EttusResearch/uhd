#
# Copyright 2015,2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# This file sets up all the stuff for the config.h file

INCLUDE(CheckCXXSymbolExistsCopy)

## Check for std::log2
CHECK_CXX_SYMBOL_EXISTS(log2 cmath HAVE_LOG2)

## Macros for the version number
IF(UHD_VERSION_DEVEL)
    MATH(EXPR UHD_VERSION_ADDED "1000000 * ${UHD_VERSION_MAJOR} + 10000 * ${UHD_VERSION_API} + 100 * ${UHD_VERSION_ABI} + 99")
ELSE()
    MATH(EXPR UHD_VERSION_ADDED "1000000 * ${UHD_VERSION_MAJOR} + 10000 * ${UHD_VERSION_API} + 100 * ${UHD_VERSION_ABI} + ${UHD_VERSION_PATCH}")
ENDIF(UHD_VERSION_DEVEL)

## RFNoC
IF(ENABLE_RFNOC)
    ADD_DEFINITIONS(-DUHD_RFNOC_ENABLED)
ENDIF(ENABLE_RFNOC)

## make sure the code knows about config.h
ADD_DEFINITIONS(-DHAVE_CONFIG_H)

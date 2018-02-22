#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

if(MPM_DEVICE STREQUAL "n3xx")
    USRP_PERIPHS_ADD_OBJECT(dboards
        magnesium_manager.cpp
    )
endif(MPM_DEVICE STREQUAL "n3xx")

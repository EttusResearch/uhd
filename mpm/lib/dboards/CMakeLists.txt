#
# Copyright 2017 Ettus Research, National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

if(ENABLE_MAGNESIUM)
    USRP_PERIPHS_ADD_OBJECT(dboards
        magnesium_manager.cpp
    )
endif(ENABLE_MAGNESIUM)

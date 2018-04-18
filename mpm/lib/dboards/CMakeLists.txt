#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
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
elseif(ENABLE_E320)
    USRP_PERIPHS_ADD_OBJECT(dboards
        neon_manager.cpp
    )
endif(ENABLE_MAGNESIUM)

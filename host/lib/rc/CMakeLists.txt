#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

include(CMakeRC)
cmrc_add_resource_library(uhd-resources
    ALIAS uhd_rc
    NAMESPACE rc
    cal/test.cal
)
set_property(TARGET uhd-resources PROPERTY POSITION_INDEPENDENT_CODE ON)

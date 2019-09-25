#
# Copyright 2010-2014,2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#


include(UHDVersion)

###############################################################################
# Set all version info equivalent to UHD versions
###############################################################################
set(MPM_VERSION_MAJOR ${UHD_VERSION_MAJOR})
set(MPM_VERSION_API   ${UHD_VERSION_API})
set(MPM_VERSION_ABI   ${UHD_VERSION_ABI})
set(MPM_VERSION_PATCH ${UHD_VERSION_PATCH})
set(MPM_VERSION_DEVEL ${UHD_VERSION_DEVEL})
set(MPM_GIT_BRANCH    ${UHD_GIT_BRANCH})
set(MPM_GIT_COUNT     ${UHD_GIT_COUNT})
set(MPM_GIT_HASH      ${UHD_GIT_HASH})
string(REPLACE "g" "" MPM_GIT_HASH_RAW ${UHD_GIT_HASH})

if(DEFINED MPM_VERSION)
    set(MPM_VERSION "${MPM_VERSION}" CACHE STRING "Set MPM_VERSION to a custom value")
elseif(TRIM_UHD_VERSION STREQUAL "True")
    set(MPM_VERSION "${MPM_VERSION_MAJOR}.${MPM_VERSION_API}.${MPM_VERSION_ABI}.${MPM_VERSION_PATCH}-${MPM_GIT_HASH}")
else()
    set(MPM_VERSION "${MPM_VERSION_MAJOR}.${MPM_VERSION_API}.${MPM_VERSION_ABI}.${MPM_VERSION_PATCH}-${MPM_GIT_COUNT}-${MPM_GIT_HASH}")
endif()

set(UHD_COMPONENT "MPM")
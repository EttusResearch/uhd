#
# Copyright 2010-2014,2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#


INCLUDE(UHDVersion)

###############################################################################
# Set all version info equivalent to UHD versions
###############################################################################
SET(MPM_VERSION_MAJOR ${UHD_VERSION_MAJOR})
SET(MPM_VERSION_API   ${UHD_VERSION_API})
SET(MPM_VERSION_ABI   ${UHD_VERSION_ABI})
SET(MPM_VERSION_PATCH ${UHD_VERSION_PATCH})
SET(MPM_VERSION_DEVEL ${UHD_VERSION_DEVEL})
SET(MPM_GIT_BRANCH    ${UHD_GIT_BRANCH})
SET(MPM_GIT_COUNT     ${UHD_GIT_COUNT})
SET(MPM_GIT_HASH      ${UHD_GIT_HASH})
STRING(REPLACE "g" "" MPM_GIT_HASH_RAW ${UHD_GIT_HASH})

IF(DEFINED MPM_VERSION)
    SET(MPM_VERSION "${MPM_VERSION}" CACHE STRING "Set MPM_VERSION to a custom value")
ELSEIF(TRIM_UHD_VERSION STREQUAL "True")
    SET(MPM_VERSION "${MPM_VERSION_MAJOR}.${MPM_VERSION_API}.${MPM_VERSION_ABI}.${MPM_VERSION_PATCH}-${MPM_GIT_HASH}")
ELSE()
    SET(MPM_VERSION "${MPM_VERSION_MAJOR}.${MPM_VERSION_API}.${MPM_VERSION_ABI}.${MPM_VERSION_PATCH}-${MPM_GIT_COUNT}-${MPM_GIT_HASH}")
ENDIF()


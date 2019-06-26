#
# Copyright 2019 Ettus Research, A National Instrument Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
set(RFNOC_PKG_FILES ${RFNOC_PKG_FILES})
set(RFNOC_PKG_TOP_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/rfnoc_image_core.v.mako
)
list(APPEND RFNOC_PKG_FILES ${RFNOC_PKG_TOP_FILES})
add_subdirectory(modules)
set(RFNOC_PKG_FILES ${RFNOC_PKG_FILES} PARENT_SCOPE)

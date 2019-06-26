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
    ${CMAKE_CURRENT_SOURCE_DIR}/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/image_builder.py
    ${CMAKE_CURRENT_SOURCE_DIR}/yaml_utils.py
)
list(APPEND RFNOC_PKG_FILES ${RFNOC_PKG_TOP_FILES})
add_subdirectory(templates)
set(RFNOC_PKG_FILES ${RFNOC_PKG_FILES} PARENT_SCOPE)

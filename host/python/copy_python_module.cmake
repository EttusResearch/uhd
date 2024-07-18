#
# Copyright 2024 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
set(BINARY_DIR "" CACHE STRING "")
set(SOURCE_DIR "" CACHE STRING "")
file(COPY "${SOURCE_DIR}/uhd/" DESTINATION "${BINARY_DIR}/uhd"
    FILES_MATCHING PATTERN "*.py" PATTERN "*.mako" PATTERN "*.yml")

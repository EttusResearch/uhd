#
# Copyright 2013-2015 Ettus Research LLC
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# Conditionally configure the OctoClock support
########################################################################
IF(ENABLE_OCTOCLOCK)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_eeprom.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_image_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_uart.cpp
    )
ENDIF(ENABLE_OCTOCLOCK)

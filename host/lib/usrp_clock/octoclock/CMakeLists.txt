#
# Copyright 2013-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Conditionally configure the OctoClock support
########################################################################
if(ENABLE_OCTOCLOCK)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_eeprom.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_image_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_uart.cpp
    )
endif(ENABLE_OCTOCLOCK)

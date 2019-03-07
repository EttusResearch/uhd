#
# Copyright 2011-2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Conditionally configure the B100 support
########################################################################
if(ENABLE_B100)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/b100_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/clock_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/codec_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dboard_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/io_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/mb_eeprom.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/usb_zero_copy_wrapper.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/fifo_ctrl_excelsior.cpp
    )
endif(ENABLE_B100)

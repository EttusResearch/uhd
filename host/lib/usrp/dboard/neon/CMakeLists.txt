#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

if(ENABLE_E320)
    list(APPEND NEON_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/neon_radio_ctrl_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/neon_radio_ctrl_init.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/neon_ad9361_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/neon_bands.cpp
    )
    LIBUHD_APPEND_SOURCES(${NEON_SOURCES})
endif(ENABLE_E320)

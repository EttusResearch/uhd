#
# Copyright 2011-2013 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

IF(ENABLE_B100 OR ENABLE_USRP2)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/gpio_core_200.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/i2c_core_200.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/rx_dsp_core_200.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/time64_core_200.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/tx_dsp_core_200.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/user_settings_core_200.cpp
    )
ENDIF(ENABLE_B100 OR ENABLE_USRP2)

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/dma_fifo_core_3000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dsp_core_utils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio_atr_3000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/i2c_core_100_wb32.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/radio_ctrl_core_3000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rx_dsp_core_3000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rx_frontend_core_200.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rx_frontend_core_3000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rx_vita_core_3000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spi_core_3000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/time_core_3000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tx_dsp_core_3000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tx_frontend_core_200.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tx_vita_core_3000.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/user_settings_core_3000.cpp
)

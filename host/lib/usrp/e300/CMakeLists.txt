#
# Copyright 2013-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Conditionally configure the USRP-E300 support
########################################################################
find_package(UDev)

IF(ENABLE_E300)
    LIST(APPEND E300_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_io_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_fifo_config.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_sysfs_hooks.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_network.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_global_regs.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_spi.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_sensor_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_i2c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_eeprom_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_common.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e300_remote_codec_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/e3xx_radio_ctrl_impl.cpp
    )
    LIBUHD_APPEND_SOURCES(${E300_SOURCES})
    IF(UDEV_FOUND AND NOT E300_FORCE_NETWORK)
        INCLUDE_DIRECTORIES(${UDEV_INCLUDE_DIR})
        LIBUHD_APPEND_LIBS(${UDEV_LIBS})
        SET_SOURCE_FILES_PROPERTIES(
            ${E300_SOURCES}
            PROPERTIES COMPILE_DEFINITIONS "E300_NATIVE=1"
        )
    ENDIF(UDEV_FOUND AND NOT E300_FORCE_NETWORK)

    IF(ENABLE_GPSD)
        SET_SOURCE_FILES_PROPERTIES(
            ${CMAKE_CURRENT_SOURCE_DIR}/e300_impl.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/e300_impl.hpp
            ${CMAKE_CURRENT_SOURCE_DIR}/e3xx_radio_ctrl_impl.hpp
            PROPERTIES COMPILE_DEFINITIONS "E300_GPSD=1"
        )
    ENDIF(ENABLE_GPSD)
ENDIF(ENABLE_E300)

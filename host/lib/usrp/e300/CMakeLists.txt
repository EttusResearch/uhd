#
# Copyright 2013-2015 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Conditionally configure the USRP-E300 support
########################################################################
find_package(UDev)

LIBUHD_REGISTER_COMPONENT("E300" ENABLE_E300 OFF "ENABLE_LIBUHD" OFF OFF)

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
            PROPERTIES COMPILE_DEFINITIONS "E300_GPSD=1"
        )
    ENDIF(ENABLE_GPSD)
ENDIF(ENABLE_E300)

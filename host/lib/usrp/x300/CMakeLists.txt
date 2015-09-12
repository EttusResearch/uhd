#
# Copyright 2013,2015 Ettus Research LLC
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
# Conditionally configure the X300 support
########################################################################
LIBUHD_REGISTER_COMPONENT("X300" ENABLE_X300 ON "ENABLE_LIBUHD" OFF OFF)

IF(ENABLE_X300)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_fw_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_fw_uart.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_adc_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_dac_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_io_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_dboard_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_clock_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_image_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/x300_adc_dac_utils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/cdecode.c
    )
ENDIF(ENABLE_X300)

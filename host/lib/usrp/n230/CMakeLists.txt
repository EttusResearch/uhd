#
# Copyright 2013 Ettus Research LLC
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
# Conditionally configure the N230 support
########################################################################
IF(ENABLE_N230)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_cores.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_resource_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_eeprom_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_stream_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_clk_pps_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_frontend_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_uart.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/n230_image_loader.cpp
   )
ENDIF(ENABLE_N230)

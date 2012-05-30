#
# Copyright 2011-2012 Ettus Research LLC
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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/gpio_core_200.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/i2c_core_100.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/i2c_core_200.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/spi_core_100.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/time64_core_200.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rx_dsp_core_200.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tx_dsp_core_200.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/rx_frontend_core_200.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tx_frontend_core_200.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/user_settings_core_200.cpp
)

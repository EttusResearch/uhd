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
# Conditionally configure the OctoClock support
########################################################################
LIBUHD_REGISTER_COMPONENT("OctoClock" ENABLE_OCTOCLOCK ON "ENABLE_LIBUHD" OFF OFF)

IF(ENABLE_OCTOCLOCK)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_eeprom.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_image_loader.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/octoclock_uart.cpp
    )
ENDIF(ENABLE_OCTOCLOCK)

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

########################################################################
# Conditionally configure the B100 support
########################################################################
LIBUHD_REGISTER_COMPONENT("B100" ENABLE_B100 ON "ENABLE_LIBUHD;ENABLE_USB" OFF)

IF(ENABLE_B100)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/b100_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/clock_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/codec_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dboard_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/io_impl.cpp
    )
ENDIF(ENABLE_B100)

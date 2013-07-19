#
# Copyright 2012-2013 Ettus Research LLC
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
LIBUHD_REGISTER_COMPONENT("B200" ENABLE_B200 ON "ENABLE_LIBUHD;ENABLE_USB" OFF)

IF(ENABLE_B200)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_iface.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_io_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/b200_uart.cpp
    )
ENDIF(ENABLE_B200)

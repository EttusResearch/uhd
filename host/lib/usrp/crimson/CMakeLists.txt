#
# Copyright 2014 Per Vices Corporation
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
# Conditionally configure the CRIMSON support
########################################################################
LIBUHD_REGISTER_COMPONENT("CRIMSON" ENABLE_CRIMSON ON "ENABLE_LIBUHD" OFF)

IF(ENABLE_CRIMSON)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/crimson_impl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/crimson_iface.cpp
    )
ENDIF(ENABLE_CRIMSON)

#
# Copyright 2016 Ettus Research
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
    ${CMAKE_CURRENT_SOURCE_DIR}/expert_container.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/expert_factory.cpp
)

# Verbose Debug output for send/recv
SET( UHD_EXPERT_LOGGING OFF CACHE BOOL "Enable verbose logging for experts" )
OPTION( UHD_EXPERT_LOGGING "Enable verbose logging for experts" "" )
IF(UHD_EXPERT_LOGGING)
    MESSAGE(STATUS "Enabling verbose logging for experts")
    ADD_DEFINITIONS(-DUHD_EXPERT_LOGGING)
ENDIF()

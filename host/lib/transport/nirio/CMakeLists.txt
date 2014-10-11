#
# Copyright 2013-2014 Ettus Research LLC
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
# Include subdirectories (different than add)
########################################################################
INCLUDE_SUBDIRECTORY(lvbitx)
INCLUDE_SUBDIRECTORY(rpc)

########################################################################
# Append to the list of sources for lib uhd
########################################################################

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/nifpga_lvbitx.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/niusrprio_session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/niriok_proxy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/niriok_proxy_impl_v1.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/niriok_proxy_impl_v2.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/nirio_resource_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/status.cpp
)

IF(WIN32)
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/nirio_driver_iface_win.cpp)
ELSEIF(${CMAKE_SYSTEM_NAME} STREQUAL "Linux") #Built-in variable encompasses all UNIX-like systems
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/nirio_driver_iface_linux.cpp)
ELSE()
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/nirio_driver_iface_unsupported.cpp)
ENDIF(WIN32)

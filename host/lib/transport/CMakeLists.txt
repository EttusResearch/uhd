#
# Copyright 2010 Ettus Research LLC
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

#This file will be included by cmake, use absolute paths!

########################################################################
# Setup defines for interface address discovery
########################################################################
MESSAGE(STATUS "Configuring interface address discovery...")

INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(ifaddrs.h HAVE_IFADDRS_H)
CHECK_INCLUDE_FILE_CXX(winsock2.h HAVE_WINSOCK2_H)

IF(HAVE_IFADDRS_H)
    MESSAGE(STATUS "  Interface address discovery supported through getifaddrs.")
    ADD_DEFINITIONS(-DHAVE_IFADDRS_H)
ELSEIF(HAVE_WINSOCK2_H)
    MESSAGE(STATUS "  Interface address discovery supported through SIO_GET_INTERFACE_LIST.")
    ADD_DEFINITIONS(-DHAVE_WINSOCK2_H)
ELSE(HAVE_IFADDRS_H)
    MESSAGE(STATUS "  Interface address discovery not supported.")
ENDIF(HAVE_IFADDRS_H)

########################################################################
# Append to the list of sources for lib uhd
########################################################################
LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/transport/gen_vrt.py
    ${CMAKE_BINARY_DIR}/lib/transport/vrt.cpp
)

LIBUHD_APPEND_SOURCES(
    ${CMAKE_SOURCE_DIR}/lib/transport/convert_types.cpp
    ${CMAKE_SOURCE_DIR}/lib/transport/if_addrs.cpp
    ${CMAKE_SOURCE_DIR}/lib/transport/udp_simple.cpp
    ${CMAKE_SOURCE_DIR}/lib/transport/udp_zero_copy_asio.cpp
)

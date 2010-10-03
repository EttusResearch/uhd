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
# Setup libusb
########################################################################
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/lib/transport)
FIND_PACKAGE(USB1 REQUIRED)

IF(LIBUSB_FOUND)
    INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIR})
    LIBUHD_APPEND_LIBS(${LIBUSB_LIBRARIES})
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_SOURCE_DIR}/lib/transport/libusb1_control.cpp
        ${CMAKE_SOURCE_DIR}/lib/transport/libusb1_zero_copy.cpp
        ${CMAKE_SOURCE_DIR}/lib/transport/libusb1_base.cpp
        ${CMAKE_SOURCE_DIR}/lib/transport/libusb1_base.hpp
    )
    IF(MSVC) #include our custom stdint for libusb
        INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/transport/msvc)
    ENDIF(MSVC)
    SET(HAVE_USB_SUPPORT TRUE)
ELSE(LIBUSB_FOUND)
    #TODO dummy usb
ENDIF(LIBUSB_FOUND)

IF(HAVE_USB_SUPPORT)
    MESSAGE(STATUS "Has USB support - found")
ELSE(HAVE_USB_SUPPORT)
    MESSAGE(STATUS "Has USB support - not found")
ENDIF(HAVE_USB_SUPPORT)

########################################################################
# Check for SIMD headers
########################################################################
INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(emmintrin.h HAVE_EMMINTRIN_H)

IF(HAVE_EMMINTRIN_H)
    ADD_DEFINITIONS(-DHAVE_EMMINTRIN_H)
ENDIF(HAVE_EMMINTRIN_H)

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
    ${CMAKE_SOURCE_DIR}/lib/transport/gen_vrt_if_packet.py
    ${CMAKE_BINARY_DIR}/lib/transport/vrt_if_packet.cpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_SOURCE_DIR}/lib/transport/gen_convert_types.py
    ${CMAKE_BINARY_DIR}/lib/transport/convert_types.cpp
)

# append this directory to the include path so the generated convert types
# can include the implementation convert types file in the source directory
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/lib/transport)

# make the generated convert types depend on the implementation header
SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_BINARY_DIR}/lib/transport/convert_types.cpp PROPERTIES
    OBJECT_DEPENDS ${CMAKE_SOURCE_DIR}/lib/transport/convert_types_impl.hpp
)

LIBUHD_APPEND_SOURCES(
    ${CMAKE_SOURCE_DIR}/lib/transport/if_addrs.cpp
    ${CMAKE_SOURCE_DIR}/lib/transport/udp_simple.cpp
    #${CMAKE_SOURCE_DIR}/lib/transport/udp_zero_copy_asio.cpp
    ${CMAKE_SOURCE_DIR}/lib/transport/vrt_packet_handler.hpp
    ${CMAKE_SOURCE_DIR}/lib/transport/zero_copy.cpp
)

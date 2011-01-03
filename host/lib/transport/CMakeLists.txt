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

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Setup libusb
########################################################################
MESSAGE(STATUS "")
FIND_PACKAGE(USB1 REQUIRED)

LIBUHD_REGISTER_COMPONENT("USB" ENABLE_USB ON "ENABLE_LIBUHD;LIBUSB_FOUND" OFF)

IF(ENABLE_USB)
    MESSAGE(STATUS "USB support enabled via libusb.")
    INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIR})
    LIBUHD_APPEND_LIBS(${LIBUSB_LIBRARIES})
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_control.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_zero_copy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_base.hpp
    )
    IF(MSVC) #include our custom stdint for libusb
        INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/msvc)
    ENDIF(MSVC)
ELSE(ENABLE_USB)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/usb_dummy_impl.cpp
    )
ENDIF(ENABLE_USB)

########################################################################
# Check for SIMD headers
########################################################################
MESSAGE(STATUS "")

INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(emmintrin.h HAVE_EMMINTRIN_H)

IF(HAVE_EMMINTRIN_H)
    ADD_DEFINITIONS(-DHAVE_EMMINTRIN_H)
ENDIF(HAVE_EMMINTRIN_H)

INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(arm_neon.h HAVE_ARM_NEON_H)

IF(HAVE_ARM_NEON_H)
    ADD_DEFINITIONS(-DHAVE_ARM_NEON_H)
ENDIF(HAVE_ARM_NEON_H)

########################################################################
# Setup defines for interface address discovery
########################################################################
MESSAGE(STATUS "")
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
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_vrt_if_packet.py
    ${CMAKE_CURRENT_BINARY_DIR}/vrt_if_packet.cpp
)

LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_convert_types.py
    ${CMAKE_CURRENT_BINARY_DIR}/convert_types.cpp
)

# append this directory to the include path so the generated convert types
# can include the implementation convert types file in the source directory
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# make the generated convert types depend on the implementation header
SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_BINARY_DIR}/convert_types.cpp PROPERTIES
    OBJECT_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/convert_types_impl.hpp
)

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/if_addrs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/udp_simple.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/udp_zero_copy_asio.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/vrt_packet_handler.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/zero_copy.cpp
)

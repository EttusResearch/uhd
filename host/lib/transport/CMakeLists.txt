#
# Copyright 2010-2011 Ettus Research LLC
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
FIND_PACKAGE(USB1)

LIBUHD_REGISTER_COMPONENT("USB" ENABLE_USB ON "ENABLE_LIBUHD;LIBUSB_FOUND" OFF)

IF(ENABLE_USB)
    MESSAGE(STATUS "USB support enabled via libusb.")
    INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIRS})
    LIBUHD_APPEND_LIBS(${LIBUSB_LIBRARIES})
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_control.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_zero_copy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_base.hpp
    )
ELSE(ENABLE_USB)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/usb_dummy_impl.cpp
    )
ENDIF(ENABLE_USB)

########################################################################
# Setup defines for interface address discovery
########################################################################
MESSAGE(STATUS "")
MESSAGE(STATUS "Configuring interface address discovery...")
INCLUDE(CheckCXXSourceCompiles)
INCLUDE(CheckIncludeFileCXX)

CHECK_CXX_SOURCE_COMPILES("
    #include <ifaddrs.h>
    int main(){
        struct ifaddrs *ifap;
        getifaddrs(&ifap);
        return 0;
    }
    " HAVE_GETIFADDRS
)

CHECK_INCLUDE_FILE_CXX(winsock2.h HAVE_WINSOCK2_H)

IF(HAVE_GETIFADDRS)
    MESSAGE(STATUS "  Interface address discovery supported through getifaddrs.")
    SET(IF_ADDRS_DEFS HAVE_GETIFADDRS)
ELSEIF(HAVE_WINSOCK2_H)
    MESSAGE(STATUS "  Interface address discovery supported through SIO_GET_INTERFACE_LIST.")
    SET(IF_ADDRS_DEFS HAVE_SIO_GET_INTERFACE_LIST)
ELSE()
    MESSAGE(STATUS "  Interface address discovery not supported.")
    SET(IF_ADDRS_DEFS HAVE_IF_ADDRS_DUMMY)
ENDIF()

SET_SOURCE_FILES_PROPERTIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/if_addrs.cpp
    PROPERTIES COMPILE_DEFINITIONS "${IF_ADDRS_DEFS}"
)

########################################################################
# Setup UDP
########################################################################
IF(WIN32)
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/udp_wsa_zero_copy.cpp)
ELSE()
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/udp_zero_copy.cpp)
ENDIF()

#On windows, the boost asio implementation uses the winsock2 library.
#Note: we exclude the .lib extension for cygwin and mingw platforms.
IF(WIN32)
    LIBUHD_APPEND_LIBS(ws2_32)
ENDIF()

#atlbase.h is not included with visual studio express
#conditionally check for atlbase.h and define if found
INCLUDE(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(atlbase.h HAVE_ATLBASE_H)
IF(HAVE_ATLBASE_H)
    SET_SOURCE_FILES_PROPERTIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/udp_zero_copy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/udp_wsa_zero_copy.cpp
        PROPERTIES COMPILE_DEFINITIONS "HAVE_ATLBASE_H"
    )
ENDIF(HAVE_ATLBASE_H)

########################################################################
# Append to the list of sources for lib uhd
########################################################################
LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_vrt_if_packet.py
    ${CMAKE_CURRENT_BINARY_DIR}/vrt_if_packet.cpp
)

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/if_addrs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/udp_simple.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/usb_zero_copy_wrapper.cpp
)

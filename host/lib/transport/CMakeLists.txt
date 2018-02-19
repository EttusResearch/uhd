#
# Copyright 2010-2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Include subdirectories (different than add)
########################################################################
IF(ENABLE_X300)
    INCLUDE_SUBDIRECTORY(nirio)
ENDIF(ENABLE_X300)

########################################################################
# Setup libusb
########################################################################
IF(ENABLE_USB)
    MESSAGE(STATUS "")
    MESSAGE(STATUS "USB support enabled via libusb.")
    INCLUDE_DIRECTORIES(${LIBUSB_INCLUDE_DIRS})
    LIBUHD_APPEND_LIBS(${LIBUSB_LIBRARIES})
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_control.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_zero_copy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_base.hpp
    )
    SET_SOURCE_FILES_PROPERTIES(
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_zero_copy.cpp
        PROPERTIES COMPILE_DEFINITIONS "${LIBUSB_DEFINITIONS}"
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
    ${CMAKE_CURRENT_SOURCE_DIR}/zero_copy_recv_offload.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_zero_copy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/if_addrs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/udp_simple.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/muxed_zero_copy_if.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/zero_copy_flow_ctrl.cpp
)

IF(ENABLE_X300)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/nirio_zero_copy.cpp
    )
ENDIF(ENABLE_X300)

IF(ENABLE_LIBERIO)
    MESSAGE(STATUS "")
    MESSAGE(STATUS "liberio support enabled.")
    INCLUDE_DIRECTORIES(${LIBERIO_INCLUDE_DIRS})
    LIBUHD_APPEND_LIBS(${LIBERIO_LIBRARIES})
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/liberio_zero_copy.cpp
    )
ENDIF(ENABLE_LIBERIO)

# Verbose Debug output for send/recv
SET( UHD_TXRX_DEBUG_PRINTS OFF CACHE BOOL "Use verbose debug output for send/recv" )
OPTION( UHD_TXRX_DEBUG_PRINTS "Use verbose debug output for send/recv" "" )
IF(UHD_TXRX_DEBUG_PRINTS)
	MESSAGE(STATUS "Using verbose debug output for send/recv")
	ADD_DEFINITIONS(-DUHD_TXRX_DEBUG_PRINTS)
ENDIF()


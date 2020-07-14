#
# Copyright 2010-2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

########################################################################
# Include subdirectories (different than add)
########################################################################
if(ENABLE_X300)
    INCLUDE_SUBDIRECTORY(nirio)
endif(ENABLE_X300)

########################################################################
# Setup libusb
########################################################################
if(ENABLE_USB)
    message(STATUS "")
    message(STATUS "USB support enabled via libusb.")
    include_directories(${LIBUSB_INCLUDE_DIRS})
    LIBUHD_APPEND_LIBS(${LIBUSB_LIBRARIES})
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_control.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_zero_copy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_base.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_base.hpp
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/libusb1_zero_copy.cpp
        PROPERTIES COMPILE_DEFINITIONS "${LIBUSB_DEFINITIONS}"
    )
else(ENABLE_USB)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/usb_dummy_impl.cpp
    )
endif(ENABLE_USB)

########################################################################
# Setup defines for interface address discovery
########################################################################
message(STATUS "")
message(STATUS "Configuring interface address discovery...")
include(CheckCXXSourceCompiles)
include(CheckIncludeFileCXX)

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

if(HAVE_GETIFADDRS)
    message(STATUS "  Interface address discovery supported through getifaddrs.")
    set(IF_ADDRS_DEFS HAVE_GETIFADDRS)
elseif(HAVE_WINSOCK2_H)
    message(STATUS "  Interface address discovery supported through SIO_GET_INTERFACE_LIST.")
    set(IF_ADDRS_DEFS HAVE_SIO_GET_INTERFACE_LIST)
else()
    message(STATUS "  Interface address discovery not supported.")
    set(IF_ADDRS_DEFS HAVE_IF_ADDRS_DUMMY)
endif()

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/if_addrs.cpp
    PROPERTIES COMPILE_DEFINITIONS "${IF_ADDRS_DEFS}"
)

########################################################################
# Setup UDP
########################################################################
if(WIN32)
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/udp_wsa_zero_copy.cpp)
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/udp_boost_asio_link.cpp)
else()
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/udp_zero_copy.cpp)
    LIBUHD_APPEND_SOURCES(${CMAKE_CURRENT_SOURCE_DIR}/udp_boost_asio_link.cpp)
endif()

#On windows, the boost asio implementation uses the winsock2 library.
#Note: we exclude the .lib extension for cygwin and mingw platforms.
if(WIN32)
    LIBUHD_APPEND_LIBS(ws2_32)
endif()

#atlbase.h is not included with visual studio express
#conditionally check for atlbase.h and define if found
include(CheckIncludeFileCXX)
CHECK_INCLUDE_FILE_CXX(atlbase.h HAVE_ATLBASE_H)
if(HAVE_ATLBASE_H)
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/udp_zero_copy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/udp_wsa_zero_copy.cpp
        PROPERTIES COMPILE_DEFINITIONS "HAVE_ATLBASE_H"
    )
endif(HAVE_ATLBASE_H)

########################################################################
# Append to the list of sources for lib uhd
########################################################################
LIBUHD_PYTHON_GEN_SOURCE(
    ${CMAKE_CURRENT_SOURCE_DIR}/gen_vrt_if_packet.py
    ${CMAKE_CURRENT_BINARY_DIR}/vrt_if_packet.cpp
)

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/zero_copy_flow_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tcp_zero_copy.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/buffer_pool.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/if_addrs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/udp_simple.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/chdr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/muxed_zero_copy_if.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/zero_copy_flow_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/inline_io_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/offload_io_service.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adapter.cpp
)

if(ENABLE_X300)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/nirio_link.cpp
    )
endif(ENABLE_X300)


if(ENABLE_DPDK)
    INCLUDE_SUBDIRECTORY(uhd-dpdk)
    include_directories(${DPDK_INCLUDE_DIRS})

    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/udp_dpdk_link.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/dpdk_simple.cpp
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/udp_dpdk_link.cpp
        PROPERTIES COMPILE_FLAGS "${UHD_DPDK_CFLAGS} -D_GNU_SOURCE"
    )
endif(ENABLE_DPDK)


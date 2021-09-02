#
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################

macro(CATALINA_APPEND_SOURCES)
    set(catalina_sources ${catalina_sources})
    list(APPEND catalina_sources ${ARGV})
    set(catalina_sources ${catalina_sources} PARENT_SCOPE)
endmacro(CATALINA_APPEND_SOURCES)

set (CMAKE_CATALINA_SOURCE_DIR
    ${UHD_SOURCE_DIR}/lib/usrp/common
)

########################################################################
# Setup defines for process scheduling (needed for thread.cpp)
########################################################################

message(STATUS "  Priority scheduling disabled.")
set(THREAD_PRIO_DEFS HAVE_THREAD_PRIO_DUMMY)

set_source_files_properties(
    ${UHD_SOURCE_DIR}/lib/utils/thread.cpp
    PROPERTIES COMPILE_DEFINITIONS "${THREAD_PRIO_DEFS}"
)

########################################################################
# Setup UHD_VERSION and UHD_COMPONENT for version.cpp
########################################################################
configure_file(
    ${UHD_SOURCE_DIR}/include/uhd/version.hpp.in
    ${CMAKE_BINARY_DIR}/include/uhd/version.hpp
)
configure_file(
    ${UHD_SOURCE_DIR}/lib/version.cpp
    ${CMAKE_BINARY_DIR}/lib/version.cpp
)

# TODO: Do something better than listing all the files individually
set(catalina_sources
    ${CMAKE_CATALINA_SOURCE_DIR}/ad9361_ctrl.cpp
    ${CMAKE_CATALINA_SOURCE_DIR}/ad936x_manager.cpp
    ${CMAKE_CATALINA_SOURCE_DIR}/ad9361_driver/ad9361_device.cpp
    ${UHD_SOURCE_DIR}/lib/types/filters.cpp
    ${UHD_SOURCE_DIR}/lib/types/ranges.cpp
    ${UHD_SOURCE_DIR}/lib/types/sensors.cpp
    ${UHD_SOURCE_DIR}/lib/types/serial.cpp
    ${UHD_SOURCE_DIR}/lib/property_tree.cpp
    ${UHD_SOURCE_DIR}/lib/utils/log.cpp
    ${UHD_SOURCE_DIR}/lib/utils/thread.cpp
    ${CMAKE_BINARY_DIR}/lib/version.cpp
)

USRP_PERIPHS_ADD_OBJECT(catalina ${catalina_sources})

target_include_directories(catalina PUBLIC
                           $<BUILD_INTERFACE:${UHD_SOURCE_DIR}/lib/include>)
target_include_directories(catalina PUBLIC
                           $<BUILD_INTERFACE:${UHD_SOURCE_DIR}/lib/usrp/common/ad9361_driver>)

target_include_directories(catalina PUBLIC
                           $<BUILD_INTERFACE:${UHD_SOURCE_DIR}/lib/include>)
target_include_directories(dboards PUBLIC
                           $<BUILD_INTERFACE:${UHD_SOURCE_DIR}/lib/usrp/common/ad9361_driver>)

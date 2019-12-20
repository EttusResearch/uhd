# Copyright 2010-2013 Ettus Research LLC
# Copyright 2019 Ettus Research, a National Instruments brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8)
project(ettus-dissector C)
enable_testing()

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

########################################################################
# Compiler specific setup
########################################################################
#if(CMAKE_COMPILER_IS_GNUC AND NOT WIN32)
#    #http://gcc.gnu.org/wiki/Visibility
#    add_definitions(-fvisibility=visible)
#endif()

find_package(GLIB2)
if(NOT GLIB2_FOUND)
    message(FATAL_ERROR "Glib not found")
endif()

find_package(Wireshark)
if(NOT WIRESHARK_FOUND)
    message(FATAL_ERROR "Wireshark dev package not found")
endif()

########################################################################
# Setup the include and linker paths
########################################################################
include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_SOURCE_DIR}
    ${WIRESHARK_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIRS}
)

set(CMAKE_INSTALL_LIBDIR ~/.wireshark)

if(${WIRESHARK_VERSION_MAJOR} VERSION_EQUAL 1)
  include(WS1.cmake)
elseif(${WIRESHARK_VERSION_MAJOR} VERSION_EQUAL 2)
  include(WS2.cmake)
elseif(${WIRESHARK_VERSION_MAJOR} VERSION_EQUAL 3)
  include(WS3.cmake)
else()
  message(FATAL_ERROR "Unknown Wireshark version ${WIRESHARK_VERSION}")
endif()

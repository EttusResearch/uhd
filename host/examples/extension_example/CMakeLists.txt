#
# Copyright 2014-2015 Ettus Research LLC
# Copyright 2022 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

cmake_minimum_required(VERSION 3.5.1)
project(EXTENSION_EXAMPLE CXX)

### Configure Compiler ########################################################
set(CMAKE_CXX_STANDARD 11)

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_EXE_LINKER_FLAGS "-lthr ${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}")
endif()

### Set up build environment ##################################################
# Choose a static or shared-library build (shared is default, and static will
# probably need some special care!)
# Set this to ON in order to link a static build of UHD:
option(UHD_USE_STATIC_LIBS OFF)

# To add UHD as a dependency to this project, add a line such as this:
find_package(UHD 4.3.0 REQUIRED)
# The version in  ^^^^^  here is a minimum version.
# We assume that in UHD 4.3 all requirements are met, however additional checks
# for extensions.hpp could be performed if desired.
# To specify an exact version:
#find_package(UHD 4.0.0 EXACT REQUIRED)

# This example also requires Boost.
# Set components here, then include UHDBoost to do the actual finding
set(UHD_BOOST_REQUIRED_COMPONENTS
    program_options
    system
    thread
)
set(BOOST_MIN_VERSION 1.71)
include(UHDBoost)

# need these include and link directories for the build
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${UHD_INCLUDE_DIRS}
)
link_directories(${Boost_LIBRARY_DIRS})

add_subdirectory(lib)

list(APPEND extexample_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/extension_example.cpp
)
add_subdirectory(include)

message(STATUS "UHD Module Path (to which the extension will be linked):")
message(STATUS ${UHD_MODULE_PATH})

### Make the executable #######################################################
add_library(extexample SHARED ${extexample_sources})

### Once it's built... ########################################################
# Here, you have commands to install your program.

set(extension_target ${CMAKE_INSTALL_PREFIX}/lib/libextexample.so)

install(TARGETS extexample DESTINATION lib)
install(CODE "execute_process(COMMAND bash -c \"mkdir -p ${UHD_MODULE_PATH} && ln -s -f ${extension_target} -t ${UHD_MODULE_PATH}\")")

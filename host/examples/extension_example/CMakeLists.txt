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
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${UHD_INCLUDE_DIRS}
)
link_directories(${Boost_LIBRARY_DIRS})

add_subdirectory(lib)

list(APPEND extexample_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/extension_example.cpp
)
add_subdirectory(include)

message(STATUS ${PROJECT_NAME} " linking against library found at:")
message(STATUS "  " ${UHD_LIB_PATH})

### Make the executable #######################################################
add_library(extexample SHARED ${extexample_sources})

##set(CMAKE_BUILD_TYPE "Release")
message(STATUS "******************************************************************************")
message(STATUS "* NOTE: When building your own app, you probably need all kinds of different  ")
message(STATUS "* compiler flags. This is just an example, so it's unlikely these settings    ")
message(STATUS "* exactly match what you require. Make sure to double-check compiler and     ")
message(STATUS "* linker flags to make sure your specific requirements are included.          ")
message(STATUS "******************************************************************************")

# Shared library case: All we need to do is link against the library, and
# anything else we need (in this case, some Boost libraries):
if(NOT UHD_USE_STATIC_LIBS)
    message(STATUS "Linking against shared UHD library.")
    target_link_libraries(extexample ${UHD_LIBRARIES} ${Boost_LIBRARIES})
# Shared library case: All we need to do is link against the library, and
# anything else we need (in this case, some Boost libraries):
else(NOT UHD_USE_STATIC_LIBS)
    message(STATUS "Linking against static UHD library.")
    target_link_libraries(extexample
        # We could use ${UHD_LIBRARIES}, but linking requires some extra flags,
        # so we use this convenience variable provided to us
        ${UHD_STATIC_LIB_LINK_FLAG}
        # Also, when linking statically, we need to pull in all the deps for
        # UHD as well, because the dependencies don't get resolved automatically
        ${UHD_STATIC_LIB_DEPS}
    )
endif(NOT UHD_USE_STATIC_LIBS)

### Once it's built... ########################################################
# Here, you have commands to install your program.

if(UNIX)
    # On Linux OS install into folder and symlink to UHD_MODULE_PATH
    set(extension_target ${CMAKE_INSTALL_PREFIX}/lib/libextexample.so)
    install(TARGETS extexample DESTINATION lib)
    install(CODE "execute_process(COMMAND bash -c \"mkdir -p ${UHD_MODULE_PATH} && ln -s -f ${extension_target} -t ${UHD_MODULE_PATH}\")")
else(UNIX)
    # on other OS (Windows) install into UHD_MODULE_PATH
    install(TARGETS extexample DESTINATION ${UHD_MODULE_PATH})
endif(UNIX)


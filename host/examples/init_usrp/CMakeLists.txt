#
# Copyright 2014-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

cmake_minimum_required(VERSION 2.8)

### Set up build environment ##################################################
# Choose a static or shared-library build (shared is default, and static will
# probably need some special care!)
# Set this to ON in order to link a static build of UHD:
option(UHD_USE_STATIC_LIBS OFF)

# This example also requires Boost:
set(BOOST_REQUIRED_COMPONENTS
    program_options
    system
    thread
)
if(MSVC)
    set(BOOST_ALL_DYN_LINK "${BOOST_ALL_DYN_LINK}" CACHE BOOL "boost enable dynamic linking")
    if(BOOST_ALL_DYN_LINK)
        add_definitions(-DBOOST_ALL_DYN_LINK) #setup boost auto-linking in msvc
    else(BOOST_ALL_DYN_LINK)
        set(BOOST_REQUIRED_COMPONENTS) #empty components list for static link
    endif(BOOST_ALL_DYN_LINK)
endif(MSVC)
find_package(Boost "1.46" REQUIRED ${BOOST_REQUIRED_COMPONENTS})

# To add UHD as a dependency to this project, add a line such as this:
find_package(UHD "3.8.0" REQUIRED)
# The version in  ^^^^^  here is a minimum version.
# To specify an exact version:
#find_package(UHD 3.8.1 EXACT REQUIRED)

### Configure Compiler ########################################################
if(CMAKE_VERSION VERSION_LESS "3.1")
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        if("${IS_APPLE}" STREQUAL "")
            set(CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
        else()
            set(CMAKE_CXX_FLAGS "-std=c++11 -stdlib=libc++ ${CMAKE_CXX_FLAGS}")
        endif()
    endif()
else()
    set(CMAKE_CXX_STANDARD 11)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" AND ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    set(CMAKE_EXE_LINKER_FLAGS "-lthr ${CMAKE_EXE_LINKER_FLAGS}")
    set(CMAKE_CXX_FLAGS "-stdlib=libc++ ${CMAKE_CXX_FLAGS}")
endif()

include_directories(
    ${Boost_INCLUDE_DIRS}
    ${UHD_INCLUDE_DIRS}
)
link_directories(${Boost_LIBRARY_DIRS})

### Make the executable #######################################################
add_executable(init_usrp init_usrp.cpp)

set(CMAKE_BUILD_TYPE "Release")
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
    target_link_libraries(init_usrp ${UHD_LIBRARIES} ${Boost_LIBRARIES})
# Shared library case: All we need to do is link against the library, and
# anything else we need (in this case, some Boost libraries):
else(NOT UHD_USE_STATIC_LIBS)
    message(STATUS "Linking against static UHD library.")
    target_link_libraries(init_usrp
        # We could use ${UHD_LIBRARIES}, but linking requires some extra flags,
        # so we use this convenience variable provided to us
        ${UHD_STATIC_LIB_LINK_FLAG}
        # Also, when linking statically, we need to pull in all the deps for
        # UHD as well, because the dependencies don't get resolved automatically
        ${UHD_STATIC_LIB_DEPS}
    )
endif(NOT UHD_USE_STATIC_LIBS)

### Once it's built... ########################################################
# Here, you would have commands to install your program.
# We will skip these in this example.

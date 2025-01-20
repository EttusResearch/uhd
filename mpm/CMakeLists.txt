#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

cmake_minimum_required(VERSION 3.1)
project(MPM C CXX) # Also has Python, but CMake can take care of that later
# Set the default value for CMAKE_INSTALL_PREFIX to /usr
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr"
        CACHE
        PATH
        "Default installation path for MPM"
        FORCE
    )
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)
set(UHD_SOURCE_DIR ${CMAKE_SOURCE_DIR}/../host)
list(INSERT CMAKE_MODULE_PATH 0 ${UHD_SOURCE_DIR}/cmake/Modules)
########################################################################
# Setup Python API
########################################################################

# We don't need to support older versions of Python, since we will always
# deploy this on devices where we control the Python version.
# MPM might work with future versions of Python, but we make no guarantees.
set(UHD_PYTHON_MIN_VERSION    "3.7" )
set(UHD_PYBIND11_MIN_VERSION  "2.7" )

# search the Python interpreter, modules and include directories
include(UHDPython)

########################################################################
# Find Python Modules
########################################################################
UHD_PYTHON_CHECK_MODULE_VERSION(
    "Mako templates"
    "mako"
    "mako.__version__"
    "0.4.2"
    HAVE_PYTHON_MODULE_MAKO
)

########################################################################
# Version Information
########################################################################
include(MPMVersion)

########################################################################
# useful macros
########################################################################
include(MPMComponent) # enable components

macro(USRP_PERIPHS_APPEND_SOURCES)
    set(usrp_periphs_sources ${usrp_periphs_sources} PARENT_SCOPE)
    list(APPEND usrp_periphs_sources ${ARGV})
endmacro(USRP_PERIPHS_APPEND_SOURCES)

macro(USRP_PERIPHS_APPEND_OBJECTS)
    set(usrp_periphs_objects ${usrp_periphs_objects} PARENT_SCOPE)
    foreach(arg ${ARGV})
        list(APPEND usrp_periphs_objects $<TARGET_OBJECTS:${arg}>)
    endforeach(arg)
    set(usrp_periphs_objects ${usrp_periphs_objects} PARENT_SCOPE)
endmacro(USRP_PERIPHS_APPEND_OBJECTS)

macro(USRP_PERIPHS_ADD_OBJECT name)
  add_library(${name} OBJECT ${ARGN})
  set_property(TARGET ${name} PROPERTY POSITION_INDEPENDENT_CODE ON)
  USRP_PERIPHS_APPEND_OBJECTS(${name})
endmacro(USRP_PERIPHS_ADD_OBJECT)

########################################################################
# Setup Boost
########################################################################
# Note: We can keep this short, because, again, we control the build
# environment. This will not, out of the box, compile on Windows or
# other platforms.
message(STATUS "")
message(STATUS "Configuring Boost C++ Libraries...")
set(BOOST_REQUIRED_COMPONENTS
    system
    thread
    date_time
)
# Same as with Python version: MPM might work with other versions of Boost,
# but we don't make any guarantees.
set(MPM_BOOST_VERSION "1.66")

# This variable is a helper for the find_package() command below, and only
# takes effect if the build is happening on a system where the FindBoost.cmake
# file is older than ${MPM_BOOST_VERSION}, which is usually not the case.
set(Boost_ADDITIONAL_VERSIONS ${MPM_BOOST_VERSION})
find_package(Boost ${MPM_BOOST_VERSION} COMPONENTS ${BOOST_REQUIRED_COMPONENTS})

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

message(STATUS "Boost include directories: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost library directories: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")


########################################################################
# Install Dirs
########################################################################
set(LIB_SUFFIX ${LIB_SUFFIX} CACHE STRING "lib directory suffix")
set(RUNTIME_DIR bin)
set(LIBRARY_DIR lib${LIB_SUFFIX})
set(INCLUDE_DIR include)
set(PKG_DATA_DIR share/mpm)
if(NOT DEFINED PKG_LIB_DIR)
    set(PKG_LIB_DIR ${LIBRARY_DIR}/mpm)
endif()
set(PKG_DOC_DIR share/doc/mpm)
set(PKG_MAN_DIR share/man/man1)

########################################################################
# Setup library configuration
########################################################################
set(CMAKE_CXX_STANDARD 14)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-Wno-psabi" _has_no_psabi)
# -Wno-psabi lets us know if the gcc ABI changed and would cause binaries
# to have different ABIs. We don't care, since we compile the entire
# system with the same gcc. These warnings are also noisy.
if(_has_no_psabi)
    message(STATUS "Disabling psABI warnings.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
endif(_has_no_psabi)
set(MPM_ALL_DEVICES n3xx e320 e31x x4xx sim tests)
set(MPM_DEVICE "n3xx" CACHE STRING "Choose an MPM device to build")
set_property(CACHE MPM_DEVICE PROPERTY STRINGS ${MPM_ALL_DEVICES})
# Validate MPM_DEVICE
list(FIND MPM_ALL_DEVICES ${MPM_DEVICE} mpm_device_check)
if(mpm_device_check EQUAL -1)
    message(FATAL_ERROR "MPM_DEVICE must be one of ${MPM_ALL_DEVICES}! \
        Specify -DMPM_DEVICE=<device> on the command line or set MPM_DEVICE using a CMake GUI.")
endif()

# Request required components for MPM_DEVICE
if(MPM_DEVICE STREQUAL "n3xx")
    set(ENABLE_MYKONOS ON)
    set(ENABLE_MAGNESIUM ON)
elseif(MPM_DEVICE STREQUAL "e320")
    set(ENABLE_E320 ON)
elseif(MPM_DEVICE STREQUAL "e31x")
    set(ENABLE_E300 ON)
elseif(MPM_DEVICE STREQUAL "x4xx")
    set(ENABLE_X400 ON)
    set(DEVICE_LIBRARIES "metal")
elseif(MPM_DEVICE STREQUAL "sim")
    set(ENABLE_SIM TRUE)
    set(ENABLE_LIBMPM OFF)
endif()

MPM_REGISTER_COMPONENT("LibMPM" ENABLE_LIBMPM ON "Boost_FOUND" OFF ON)
MPM_REGISTER_COMPONENT("RegMaps" ENABLE_REGMAPS ON "HAVE_PYTHON_MODULE_MAKO" OFF OFF)
MPM_REGISTER_COMPONENT("Mykonos" ENABLE_MYKONOS OFF "ENABLE_LIBMPM" OFF OFF)
MPM_REGISTER_COMPONENT("Magnesium" ENABLE_MAGNESIUM OFF "ENABLE_MYKONOS" OFF OFF)
MPM_REGISTER_COMPONENT("E320" ENABLE_E320 OFF "ENABLE_LIBMPM" OFF OFF)
MPM_REGISTER_COMPONENT("E300" ENABLE_E300 OFF "ENABLE_LIBMPM" OFF OFF)
MPM_REGISTER_COMPONENT("X400" ENABLE_X400 OFF "ENABLE_LIBMPM;ENABLE_REGMAPS" OFF OFF)

if(NOT ENABLE_SIM)
    add_subdirectory(include)
    include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_BINARY_DIR}/include
        ${UHD_SOURCE_DIR}/include
    )

    add_subdirectory(lib)

    message("usrp_periphs objects: ${usrp_periphs_objects}")
    add_library(usrp-periphs SHARED ${usrp_periphs_objects})
    target_link_libraries(usrp-periphs
        udev
        ${Boost_LIBRARIES}
        ${DEVICE_LIBRARIES}
    )

    install(TARGETS usrp-periphs LIBRARY DESTINATION ${LIBRARY_DIR} COMPONENT libraries)

    # TODO: Come up with a versioning scheme for the MPM ABI. Not high priority
    # though... we're the only ones linking against that.
    set_target_properties(usrp-periphs PROPERTIES VERSION "${MPM_VERSION_MAJOR}.${MPM_VERSION_API}.${MPM_VERSION_ABI}")
    set_target_properties(usrp-periphs PROPERTIES SOVERSION ${MPM_VERSION_MAJOR})

    add_subdirectory(systemd)
    add_subdirectory(tools)
endif(NOT ENABLE_SIM)

enable_testing()
add_subdirectory(python)



########################################################################
# Print Summary
########################################################################
if(MPM_VERSION_DEVEL AND NOT MPM_GIT_BRANCH STREQUAL "maint")
    message(STATUS "******************************************************")
    if(MPM_GIT_BRANCH STREQUAL "master")
        message(STATUS "* You are building the UHD development master branch.")
        message(STATUS "* For production code, we recommend our stable,")
        message(STATUS "* releases or using the release branch (maint).")
    else()
        message(STATUS "* You are building a development branch of UHD.")
        message(STATUS "* These branches are designed to provide early access")
        message(STATUS "* to UHD and USRP features, but should be considered")
        message(STATUS "* unstable and/or experimental!")
    endif(MPM_GIT_BRANCH STREQUAL "master")
    message(STATUS "******************************************************")
endif(MPM_VERSION_DEVEL AND NOT MPM_GIT_BRANCH STREQUAL "maint")
message(STATUS "Building version: ${MPM_VERSION}")
message(STATUS "Building for device: ${MPM_DEVICE}")

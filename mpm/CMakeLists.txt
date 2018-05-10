#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(MPM C CXX) # Also has Python, but CMake can take care of that later
# Set the default value for CMAKE_INSTALL_PREFIX to /usr
IF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    SET(CMAKE_INSTALL_PREFIX "/usr"
        CACHE
        PATH
        "Default installation path for MPM"
        FORCE
    )
ENDIF(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

LIST(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)
SET(UHD_HOST_ROOT ${CMAKE_SOURCE_DIR}/../host)
########################################################################
# Setup Python API
########################################################################
SET(PYTHON_ADDITIONAL_VERSIONS 3.4 3.5)
FIND_PACKAGE(PythonInterp 3 REQUIRED)
FIND_PACKAGE(PythonLibs 3 REQUIRED)
# Now, we can also include CMake modules from UHD:
LIST(INSERT CMAKE_MODULE_PATH 0 ${UHD_HOST_ROOT}/cmake/Modules)


########################################################################
# Version Information
########################################################################
INCLUDE(MPMVersion)

########################################################################
# useful macros
########################################################################
INCLUDE(MPMComponent) # enable components

MACRO(USRP_PERIPHS_APPEND_SOURCES)
    SET(usrp_periphs_sources ${usrp_periphs_sources} PARENT_SCOPE)
    LIST(APPEND usrp_periphs_sources ${ARGV})
ENDMACRO(USRP_PERIPHS_APPEND_SOURCES)

MACRO(USRP_PERIPHS_APPEND_OBJECTS)
    SET(usrp_periphs_objects ${usrp_periphs_objects} PARENT_SCOPE)
    foreach(arg ${ARGV})
        LIST(APPEND usrp_periphs_objects $<TARGET_OBJECTS:${arg}>)
    endforeach(arg)
    SET(usrp_periphs_objects ${usrp_periphs_objects} PARENT_SCOPE)
ENDMACRO(USRP_PERIPHS_APPEND_OBJECTS)

MACRO(USRP_PERIPHS_ADD_OBJECT name)
  ADD_LIBRARY(${name} OBJECT ${ARGN})
  SET_PROPERTY(TARGET ${name} PROPERTY POSITION_INDEPENDENT_CODE ON)
  USRP_PERIPHS_APPEND_OBJECTS(${name})
ENDMACRO(USRP_PERIPHS_ADD_OBJECT)

########################################################################
# Setup Boost
########################################################################
MESSAGE(STATUS "")
MESSAGE(STATUS "Configuring Boost C++ Libraries...")
SET(BOOST_REQUIRED_COMPONENTS
  system
  python3
)

IF(MINGW)
    LIST(APPEND BOOST_REQUIRED_COMPONENTS thread_win32)
ELSE()
    LIST(APPEND BOOST_REQUIRED_COMPONENTS thread)
ENDIF()

IF(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")
    LIST(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
ENDIF(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")

IF(MSVC)
    SET(BOOST_ALL_DYN_LINK "${BOOST_ALL_DYN_LINK}" CACHE BOOL "boost enable dynamic linking")
    IF(BOOST_ALL_DYN_LINK)
        ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK) #setup boost auto-linking in msvc
    ELSE(BOOST_ALL_DYN_LINK)
        SET(BOOST_REQUIRED_COMPONENTS) #empty components list for static link
    ENDIF(BOOST_ALL_DYN_LINK)
ENDIF(MSVC)

SET(Boost_ADDITIONAL_VERSIONS
    "1.56.0" "1.56" "1.57" "1.57" "1.58" "1.59" "1.60" "1.61" "1.62" "1.63"
    "1.64"
)
FIND_PACKAGE(Boost 1.53 COMPONENTS ${BOOST_REQUIRED_COMPONENTS})

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

MESSAGE(STATUS "Boost include directories: ${Boost_INCLUDE_DIRS}")
MESSAGE(STATUS "Boost library directories: ${Boost_LIBRARY_DIRS}")
MESSAGE(STATUS "Boost libraries: ${Boost_LIBRARIES}")


########################################################################
# Install Dirs
########################################################################
#when the library suffix should be 64 (applies to redhat linux family)
IF(NOT DEFINED LIB_SUFFIX AND REDHAT AND CMAKE_SYSTEM_PROCESSOR MATCHES "64$")
    SET(LIB_SUFFIX 64)
ENDIF()
IF(CMAKE_INSTALL_LIBDIR MATCHES lib64)
    SET(LIB_SUFFIX 64)
ENDIF()

SET(LIB_SUFFIX ${LIB_SUFFIX} CACHE STRING "lib directory suffix")
SET(RUNTIME_DIR bin)
SET(LIBRARY_DIR lib${LIB_SUFFIX})
SET(INCLUDE_DIR include)
SET(PKG_DATA_DIR share/mpm)
IF(NOT DEFINED PKG_LIB_DIR)
    SET(PKG_LIB_DIR ${LIBRARY_DIR}/mpm)
ENDIF()
SET(PKG_DOC_DIR share/doc/mpm)
SET(PKG_MAN_DIR share/man/man1)

########################################################################
# Setup library configuration
########################################################################
SET(CMAKE_CXX_STANDARD 14)
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-Wno-psabi" _has_no_psabi)
if(_has_no_psabi)
    MESSAGE(STATUS "Disabling psABI warnings.")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-psabi")
endif(_has_no_psabi)
SET(MPM_ALL_DEVICES n3xx tests)
SET(MPM_DEVICE "n3xx" CACHE STRING "Choose an MPM device to build")
SET_PROPERTY(CACHE MPM_DEVICE PROPERTY STRINGS ${MPM_ALL_DEVICES})
# Validate MPM_DEVICE
list(FIND MPM_ALL_DEVICES ${MPM_DEVICE} mpm_device_check)
if(mpm_device_check EQUAL -1)
    message(FATAL_ERROR "MPM_DEVICE must be one of ${MPM_ALL_DEVICES}! \
        Specify -DMPM_DEVICE=<device> on the command line or set MPM_DEVICE using a CMake GUI.")
endif()

# Request required components for MPM_DEVICE
IF(MPM_DEVICE STREQUAL "n3xx")
    SET(ENABLE_MYKONOS ON)
    SET(ENABLE_MAGNESIUM ON)
ENDIF()

MPM_REGISTER_COMPONENT("LibMPM" ENABLE_LIBMPM ON "Boost_FOUND" OFF ON)
MPM_REGISTER_COMPONENT("Mykonos" ENABLE_MYKONOS ON "ENABLE_LIBMPM" OFF OFF)
MPM_REGISTER_COMPONENT("Magnesium" ENABLE_MAGNESIUM ON "ENABLE_MYKONOS" OFF OFF)

ADD_SUBDIRECTORY(include)
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/include
    ${UHD_HOST_ROOT}/include
)

ADD_SUBDIRECTORY(lib)

MESSAGE("usrp_periphs objects: ${usrp_periphs_objects}")
ADD_LIBRARY(usrp-periphs SHARED ${usrp_periphs_objects})
TARGET_LINK_LIBRARIES(usrp-periphs
    udev
    ${Boost_LIBRARIES}
)

IF(WIN32)
    SET(DESTINATION_KEYWORD RUNTIME)
ELSE()
    SET(DESTINATION_KEYWORD LIBRARY)
ENDIF()
INSTALL(TARGETS usrp-periphs ${DESTINATION_KEYWORD} DESTINATION ${LIBRARY_DIR} COMPONENT libraries)

# TODO: Come up with a versioning scheme for the MPM ABI. Not high priority
# though... we're the only ones linking against that.
SET_TARGET_PROPERTIES(usrp-periphs PROPERTIES VERSION "${MPM_VERSION_MAJOR}.${MPM_VERSION_API}.${MPM_VERSION_ABI}")
SET_TARGET_PROPERTIES(usrp-periphs PROPERTIES SOVERSION ${MPM_VERSION_MAJOR})

ADD_SUBDIRECTORY(python)
ADD_SUBDIRECTORY(tools)
ADD_SUBDIRECTORY(systemd)

########################################################################
# Print Summary
########################################################################
IF(MPM_VERSION_DEVEL AND NOT MPM_GIT_BRANCH STREQUAL "maint")
    MESSAGE(STATUS "******************************************************")
    IF(MPM_GIT_BRANCH STREQUAL "master")
        MESSAGE(STATUS "* You are building the UHD development master branch.")
        MESSAGE(STATUS "* For production code, we recommend our stable,")
        MESSAGE(STATUS "* releases or using the release branch (maint).")
    ELSE()
        MESSAGE(STATUS "* You are building a development branch of UHD.")
        MESSAGE(STATUS "* These branches are designed to provide early access")
        MESSAGE(STATUS "* to UHD and USRP features, but should be considered")
        MESSAGE(STATUS "* unstable and/or experimental!")
    ENDIF(MPM_GIT_BRANCH STREQUAL "master")
    MESSAGE(STATUS "******************************************************")
ENDIF(MPM_VERSION_DEVEL AND NOT MPM_GIT_BRANCH STREQUAL "maint")
MESSAGE(STATUS "Building version: ${MPM_VERSION}")
MESSAGE(STATUS "Building for device: ${MPM_DEVICE}")

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

########################################################################
# useful macros
########################################################################
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
# Setup Python API
########################################################################
SET(PYTHON_ADDITIONAL_VERSIONS 3.4 3.5)
FIND_PACKAGE(PythonInterp 3)
FIND_PACKAGE(PythonLibs 3)

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
SET(CMAKE_CXX_STANDARD 11)
SET(MPM_DEVICE "n3xx" CACHE STRING "Choose an MPM device to build")
SET_PROPERTY(CACHE MPM_DEVICE PROPERTY STRINGS n3xx tests)

SET(UHD_HOST_ROOT ${CMAKE_SOURCE_DIR}/../host)

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

SET(USRP_PERIPHS_MAJOR 0)
SET(USRP_PERIPHS_MINOR 1)
SET(USRP_PERIPHS_PATCH 0)
SET(USRP_PERIPHS_VERSION ${USRP_PERIPHS_MAJOR}.${USRP_PERIPHS_MINOR}.${USRP_PERIPHS_PATCH})

SET_TARGET_PROPERTIES(usrp-periphs PROPERTIES VERSION ${USRP_PERIPHS_VERSION})
SET_TARGET_PROPERTIES(usrp-periphs PROPERTIES SOVERSION ${USRP_PERIPHS_MAJOR})

ADD_SUBDIRECTORY(python)
ADD_SUBDIRECTORY(tools)
ADD_SUBDIRECTORY(systemd)

#
# Try to find the wireshark library and its includes
#
# This snippet sets the following variables:
#  WIRESHARK_FOUND             True if wireshark library got found
#  WIRESHARK_INCLUDE_DIRS      Location of the wireshark headers 
#  WIRESHARK_LIBRARIES         List of libraries to use wireshark
#  WIRESHARK_PLUGIN_DIR        Directory where wireshark plugins are to be installed
#  WIRESHARK_VERSION           Version of Wireshark found as X.Y.Z
#  WIRESHARK_VERSION_MAJOR     Version of Wireshark found as just X
#  WIRESHARK_VERSION_MINOR     Version of Wireshark found as just Y
#  WIRESHARK_VERSION_PATCH     Version of Wireshark found as just Z
#  WIRESHARK_PLUGIN_DIR        Top-level Wireshark plugin directory

#  Copyright (c) 2011 Reinhold Kainhofer <reinhold@kainhofer.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

set( WIRESHARK_FOUND 0 )

# wireshark may or not install its library with pkg-config
# information, so we need to be able to manually find the libraries
# and headers.

include(FindPkgConfig)
pkg_search_module( PC_WIRESHARK wireshark )

find_path( WIRESHARK_INCLUDE_DIRS
  NAMES wireshark/epan/column.h
  HINTS ${PC_WIRESHARK_INCLUDE_DIRS}
)
if(NOT WIRESHARK_INCLUDE_DIRS)
  message( SEND_ERROR "Could NOT find the wireshark headers" )
endif()
string(REGEX MATCH "(.*)/[^/]*" WS_PREFIX ${WIRESHARK_INCLUDE_DIRS})
set(WIRESHARK_PREFIX ${CMAKE_MATCH_1})
set(WIRESHARK_INCLUDE_DIRS ${WIRESHARK_INCLUDE_DIRS}/wireshark)

find_library( WIRESHARK_LIBRARIES
  NAMES wireshark
  HINTS ${PC_WIRESHARK_LIBRARY_DIRS}
  ${WIRESHARK_PREFIX}/lib
)
if(NOT WIRESHARK_LIBRARIES)
  message( SEND_ERROR "Could NOT find the wireshark library" )
endif()

set( WIRESHARK_FOUND 1 )

# if the PC version exists, use it to obtain info
if(PC_WIRESHARK_VERSION)
  set(WIRESHARK_VERSION ${PC_WIRESHARK_VERSION})
  pkg_get_variable(WIRESHARK_PLUGIN_DIR wireshark plugindir)
else()
  # determine version from "config.h"
  file(READ "${WIRESHARK_INCLUDE_DIRS}/config.h" WS_CONFIG_H)
  # retrieve "PACKET_VERSION" value
  string(REGEX MATCH "PACKAGE_VERSION \"([0-9.]*)" WS_VERSION ${WS_CONFIG_H})
  set(WIRESHARK_VERSION ${CMAKE_MATCH_1})

  # default plugin directory
  # retrieve the LIBDIR from the LIBRARIES
  string(REGEX MATCH "(.*)/[^/]*" WS_LIBDIR ${WIRESHARK_LIBRARIES})
  set(WIRESHARK_LIBDIR ${CMAKE_MATCH_1})
  set(WIRESHARK_PLUGIN_DIR "${WIRESHARK_LIBDIR}/wireshark/plugins/${WIRESHARK_VERSION}")
endif()

# split the VERSION string, regardless of where it came from
string(REGEX MATCHALL "([^.]*).([^.]*).([^.]*)" WS_VERSIONS "${WIRESHARK_VERSION}")
set(WIRESHARK_VERSION_MAJOR ${CMAKE_MATCH_1})
set(WIRESHARK_VERSION_MINOR ${CMAKE_MATCH_2})
set(WIRESHARK_VERSION_PATCH ${CMAKE_MATCH_3})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Wireshark DEFAULT_MSG WIRESHARK_LIBRARIES WIRESHARK_INCLUDE_DIRS WIRESHARK_PLUGIN_DIR)
mark_as_advanced(WIRESHARK_INCLUDE_DIRS WIRESHARK_LIBRARIES WIRESHARK_PLUGIN_DIR)

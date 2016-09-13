# Copyright 2010-2016 Ettus Research LLC
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.


########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.6)
project(octoclock-dissector C)
set(octoclock-dissector_VERSION_MAJOR 0)
set(octoclock-dissector_VERSION_MINOR 7)
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
    ${CMAKE_SOURCE_DIR}
    ${GLIB2_INCLUDE_DIRS}
    ${WIRESHARK_INCLUDE_DIRS}
)

#link_directories(
#    ${Boost_LIBRARY_DIRS}
#)
set(CMAKE_INSTALL_LIBDIR ~/.wireshark)

# Set component parameters
set(octoclock_DISSECTOR_INCLUDE_DIRS ${CMAKE_SOURCE_DIR} CACHE INTERNAL "" FORCE)

set(octoclock_PLUGIN_SRC ${CMAKE_SOURCE_DIR}/packet-octoclock.c)

set(PLUGIN_C_GENERATOR ${CMAKE_SOURCE_DIR}/make-dissector-reg.py)
set(PLUGIN_C plugin.c)

add_custom_command(
        OUTPUT ${PLUGIN_C}
        DEPENDS ${octoclock_PLUGIN_SRC}
        COMMAND ${PLUGIN_C_GENERATOR} ${CMAKE_SOURCE_DIR} plugin ${octoclock_PLUGIN_SRC}
        COMMENT "Generating ${PLUGIN_C}"
)

add_library(octoclock-plugin MODULE
    ${PLUGIN_C}
    moduleinfo.h
    ${octoclock_PLUGIN_SRC}
)
set_target_properties(octoclock-plugin PROPERTIES PREFIX "")
set_target_properties(octoclock-plugin PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")
target_link_libraries(octoclock-plugin wireshark)

install(TARGETS octoclock-plugin
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/plugins NAMELINK_SKIP
)

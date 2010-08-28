#
# Copyright 2010 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(UHD CXX C)
ENABLE_TESTING()

########################################################################
# Config Files (include order is important)
########################################################################
INCLUDE(${CMAKE_SOURCE_DIR}/config/Python.cmake)
INCLUDE(${CMAKE_SOURCE_DIR}/config/Version.cmake)
INCLUDE(${CMAKE_SOURCE_DIR}/config/CPack.cmake)

########################################################################
# Install Dirs
########################################################################
SET(RUNTIME_DIR bin)
SET(LIBRARY_DIR lib)
SET(INCLUDE_DIR include)
SET(PKG_DATA_DIR share/uhd)
SET(PKG_DOC_DIR share/doc/uhd)
MESSAGE(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")

########################################################################
# Local Include Dir
########################################################################
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

########################################################################
# Optional Compiler Flags
########################################################################
INCLUDE(CheckCXXCompilerFlag)
MACRO(UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG flag have)
    CHECK_CXX_COMPILER_FLAG(${flag} ${have})
    IF(${have})
        ADD_DEFINITIONS(${flag})
    ENDIF(${have})
ENDMACRO(UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG)

#select the release build type by default to get optimization flags
IF(NOT CMAKE_BUILD_TYPE)
   SET(CMAKE_BUILD_TYPE "Release")
   MESSAGE(STATUS "Build type not specified: defaulting to release.")
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(CMAKE_COMPILER_IS_GNUCXX)
    ADD_DEFINITIONS(-Wall)
    ADD_DEFINITIONS(-Wextra)
    #only export symbols that are declared to be part of the uhd api:
    UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(MSVC)
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501) #minimum version required is windows xp
    ADD_DEFINITIONS(-DNOMINMAX) #disables stupidity and enables std::min and std::max
    ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS) #avoid warnings from boost::split
    ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK) #setup boost auto-linking in msvc
    ADD_DEFINITIONS(/arch:SSE2 /G7) #processor optimization flags
ENDIF(MSVC)

########################################################################
# Setup Boost
########################################################################
IF(UNIX AND EXISTS "/usr/lib64")
    LIST(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
ENDIF(UNIX AND EXISTS "/usr/lib64")

SET(Boost_ADDITIONAL_VERSIONS "1.42.0" "1.42" "1.43.0" "1.43")
FIND_PACKAGE(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS
    date_time
    filesystem
    program_options
    regex
    system
    thread
    unit_test_framework
)

INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
LINK_DIRECTORIES(${Boost_LIBRARY_DIRS})

########################################################################
# Create Uninstall Target
########################################################################
CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/config/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

ADD_CUSTOM_TARGET(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

########################################################################
# Create Pkg Config File
########################################################################
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/uhd.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/uhd.pc
@ONLY)

INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/uhd.pc
    DESTINATION ${LIBRARY_DIR}/pkgconfig
)

########################################################################
# Install Package Docs
########################################################################
INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/README
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    ${CMAKE_CURRENT_SOURCE_DIR}/AUTHORS
    DESTINATION ${PKG_DOC_DIR}
)

########################################################################
# Add the subdirectories
########################################################################
ADD_SUBDIRECTORY(docs)
ADD_SUBDIRECTORY(examples)
ADD_SUBDIRECTORY(include)
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(utils)

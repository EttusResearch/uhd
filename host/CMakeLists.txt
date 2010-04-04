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
PROJECT(UHD)
ENABLE_TESTING()

########################################################################
# Setup CPack
########################################################################
SET(CPACK_PACKAGE_VERSION_MAJOR 0)
SET(CPACK_PACKAGE_VERSION_MINOR 0)
SET(CPACK_PACKAGE_VERSION_PATCH 0)
SET(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README)
SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)
INCLUDE(CPack) #include after setting vars

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
FUNCTION(UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG flag have)
    CHECK_CXX_COMPILER_FLAG(${flag} ${have})
    IF(${have})
        ADD_DEFINITIONS(${flag})
    ENDIF(${have})
ENDFUNCTION(UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG)

IF(UNIX)
    UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG(-Wall      HAVE_WALL)
    UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG(-Wextra    HAVE_WEXTRA)
    UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG(-pedantic  HAVE_PEDANTIC)
    UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG(-ansi      HAVE_ANSI)
    #only export symbols that are declared to be part of the uhd api:
    UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN)
ENDIF(UNIX)

IF(WIN32)
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501) #minimum version required is windows xp
    ADD_DEFINITIONS(-DNOMINMAX) #disables stupidity and enables std::min and std::max
    ADD_DEFINITIONS(-D_SCL_SECURE_NO_WARNINGS) #avoid warnings from boost::split
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS) #avoid warnings from std::getenv
    ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK) #setup boost auto-linking in msvc
ENDIF(WIN32)

########################################################################
# Setup Boost
########################################################################
SET(Boost_ADDITIONAL_VERSIONS "1.42.0" "1.42")
FIND_PACKAGE(Boost 1.36 REQUIRED COMPONENTS
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
# Setup Endianess
########################################################################
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(HAVE_BIG_ENDIAN)
IF(HAVE_BIG_ENDIAN)
    ADD_DEFINITIONS(-DHAVE_BIG_ENDIAN)
ENDIF(HAVE_BIG_ENDIAN)

########################################################################
# Create Uninstall Target
########################################################################
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

ADD_CUSTOM_TARGET(uninstall
  ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

########################################################################
# Create Pkg Config File
########################################################################
CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/uhd.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/uhd.pc
@ONLY)

INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/uhd.pc
    DESTINATION ${LIBRARY_DIR}/pkgconfig
)

########################################################################
# Setup Docs
########################################################################
INCLUDE(FindDoxygen)

IF(DOXYGEN_FOUND)
    SET(CMAKE_CURRENT_BINARY_DIR_DOXYGEN ${CMAKE_CURRENT_BINARY_DIR}/doxygen)
    CONFIGURE_FILE(
        ${CMAKE_SOURCE_DIR}/Doxyfile.in
        ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    @ONLY)
    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN}
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        COMMENT "Generating documentation with doxygen"
    )
    ADD_CUSTOM_TARGET(doxygen_docs ALL DEPENDS ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN})
    INSTALL(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR_DOXYGEN} DESTINATION ${PKG_DOC_DIR})
ENDIF(DOXYGEN_FOUND)

INSTALL(FILES
    ${CMAKE_SOURCE_DIR}/README
    ${CMAKE_SOURCE_DIR}/LICENSE
    DESTINATION ${PKG_DOC_DIR}
)

########################################################################
# Add the subdirectories
########################################################################
ADD_SUBDIRECTORY(examples)
ADD_SUBDIRECTORY(include)
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(utils)

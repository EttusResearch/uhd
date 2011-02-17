#
# Copyright 2010-2011 Ettus Research LLC
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

########################################################################
#IF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
#    MESSAGE(FATAL_ERROR "Prevented in-tree built. This is bad practice.")
#ENDIF(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

########################################################################
# Project setup
########################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(UHD CXX)
ENABLE_TESTING()
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/Modules)
INCLUDE(UHDComponent) #enable components
INCLUDE(UHDPackage)   #setup cpack

########################################################################
# Install Dirs
########################################################################
SET(LIB_SUFFIX ${LIB_SUFFIX} CACHE STRING "lib directory suffix")
SET(RUNTIME_DIR bin)
SET(LIBRARY_DIR lib${LIB_SUFFIX})
SET(INCLUDE_DIR include)
SET(PKG_DATA_DIR share/uhd)
SET(PKG_DOC_DIR share/doc/uhd)

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

#Creating a shared pointer itself has allocation overhead.
#Define the quick allocator to reduce fast-path overhead.
ADD_DEFINITIONS(-DBOOST_SP_USE_QUICK_ALLOCATOR)

IF(CMAKE_COMPILER_IS_GNUCXX)
    ADD_DEFINITIONS(-Wall)
    ADD_DEFINITIONS(-Wextra)
    #ADD_DEFINITIONS(-pedantic)
    #ADD_DEFINITIONS(-ansi)
    #only export symbols that are declared to be part of the uhd api:
    UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN)
ENDIF(CMAKE_COMPILER_IS_GNUCXX)

IF(MSVC)
    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/msvc)
    ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501) #minimum version required is windows xp
    ADD_DEFINITIONS(-DNOMINMAX) #disables stupidity and enables std::min and std::max
    ADD_DEFINITIONS( #stop all kinds of compatibility warnings
        -D_SCL_SECURE_NO_WARNINGS
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_SECURE_NO_DEPRECATE
        -D_CRT_NONSTDC_NO_DEPRECATE
    )
    ADD_DEFINITIONS(-DBOOST_ALL_DYN_LINK) #setup boost auto-linking in msvc
ENDIF(MSVC)

########################################################################
# Setup Boost
########################################################################
IF(UNIX AND EXISTS "/usr/lib64")
    LIST(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
ENDIF(UNIX AND EXISTS "/usr/lib64")

SET(Boost_ADDITIONAL_VERSIONS "1.42.0" "1.42" "1.43.0" "1.43" "1.44.0" "1.44")
FIND_PACKAGE(Boost ${BOOST_MIN_VERSION} COMPONENTS
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
# Check Python Modules
########################################################################
INCLUDE(UHDPython)

PYTHON_CHECK_MODULE(
    "Python version 2.6 or greater"
    "platform" "platform.python_version() >= '2.6'"
    HAVE_PYTHON_PLAT_MIN_VERSION
)

PYTHON_CHECK_MODULE(
    "Cheetah templates 2.0.0 or greater"
    "Cheetah" "Cheetah.Version >= '2.0.0'"
    HAVE_PYTHON_MODULE_CHEETAH
)

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
# Register top level components
########################################################################
LIBUHD_REGISTER_COMPONENT("LibUHD" ENABLE_LIBUHD ON "Boost_FOUND;HAVE_PYTHON_PLAT_MIN_VERSION;HAVE_PYTHON_MODULE_CHEETAH" OFF)
LIBUHD_REGISTER_COMPONENT("Examples" ENABLE_EXAMPLES ON "ENABLE_LIBUHD" OFF)
LIBUHD_REGISTER_COMPONENT("Utils" ENABLE_UTILS ON "ENABLE_LIBUHD" OFF)
LIBUHD_REGISTER_COMPONENT("Tests" ENABLE_TESTS ON "ENABLE_LIBUHD" OFF)

########################################################################
# Add the subdirectories
########################################################################
ADD_SUBDIRECTORY(docs)

IF(ENABLE_EXAMPLES)
    ADD_SUBDIRECTORY(examples)
ENDIF(ENABLE_EXAMPLES)

ADD_SUBDIRECTORY(include)

IF(ENABLE_LIBUHD)
    ADD_SUBDIRECTORY(lib)
ENDIF(ENABLE_LIBUHD)

IF(ENABLE_TESTS)
    ADD_SUBDIRECTORY(tests)
ENDIF(ENABLE_TESTS)

IF(ENABLE_UTILS)
    ADD_SUBDIRECTORY(utils)
ENDIF(ENABLE_UTILS)

ADD_SUBDIRECTORY(usrp_e_utils)

########################################################################
# Print Summary
########################################################################
UHD_PRINT_COMPONENT_SUMMARY()
MESSAGE(STATUS "Building version: ${CPACK_PACKAGE_VERSION}")
MESSAGE(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")

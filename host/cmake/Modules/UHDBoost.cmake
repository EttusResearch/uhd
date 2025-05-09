#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# The following variables can be defined external to this script.
#
# ENABLE_STATIC_LIBS : whether to enable static libraries, which will
# require static Boost libraries too. If not using static libraries,
# shared libraries will be used. The default is to use shared
# libraries, and this default will be used if this variable is not
# defined or if is it NO/OFF/FALSE.
#
# UHD_BOOST_REQUIRED_COMPONENTS : Boost components that are required
# for Boost_FOUND to be true. The linkage (shared or static) must be
# correct and the library must also be found.
#
# UHD_BOOST_OPTIONAL_COMPONENTS : Boost components that are
# optional. They do not impact Boost_FOUND. The linkage will still be
# honored, and the library must still be found for the return
# variables to be set.
#
# NOTE: If neither UHD_BOOST_REQUIRED_COMPONENTS and
# UHD_BOOST_OPTIONAL_COMPONENTS are defined and/or are empty, then
# this script just checked to see if the minimum required version of
# Boost can be found, and sets Boost_FOUND accordingly; all of the
# other return variables will be unset / undefined.
#
# UHD_BOOST_REQUIRED : Whether to set Boost required components as
# "required" during finding. If not specified, then defaults to
# TRUE. If REQUIRED is TRUE and Boost is not found, then this script
# will error out. Otherwise this script will not error out, and the
# variable Boost_FOUND will indicate whether a compatible version
# Boost was found or not.
#
# Upon find returning, the following variables will be set upon return
# of this script:
#
# Boost_FOUND : will be TRUE if all of the required Boost components
# are found with the correct linkage (static or shared); otherwise
# this variable will be FALSE unless there are no components specified
# in which case this variable will be unset.
#
# Boost_INCLUDE_DIRS : directories to use with 'include_directories',
# where the top-level "boost" directory is located for headers.
#
# Boost_LIBRARY_DIRS : directories for use in finding libraries; this
# variable is generally not used in favor of either (1) defining
# library names with the full path; or (2) using targets where the
# full path is defined already. This variable is probably deprecated.
#
# Boost_LIBRARIES : a list of found Boost libraries, each of which can
# be either (1) or (2) above; either will work with modern CMake.
#
########################################################################
message(STATUS "")
message(STATUS "Checking for Boost version ${UHD_BOOST_MIN_VERSION} or greater")

# unset return variables
unset(Boost_FOUND)
unset(Boost_INCLUDE_DIRS)
unset(Boost_LIBRARY_DIRS)
unset(Boost_LIBRARIES)

# check whether to set REQUIRED or not
# if not set, default is to require Boost
if(NOT DEFINED UHD_BOOST_REQUIRED)
    # UHD_BOOST_REQUIRED is not set; use the default
    set(UHD_BOOST_REQUIRED "REQUIRED")
elseif(UHD_BOOST_REQUIRED)
    # UHD_BOOST_REQUIRED is set to TRUE/ON/1
    set(UHD_BOOST_REQUIRED "REQUIRED")
else()
    # UHD_BOOST_REQUIRED is not set to TRUE/ON/1
    unset(UHD_BOOST_REQUIRED)
endif()

# set this for verbosity during 'find'
# set(Boost_DEBUG TRUE)

# verify we're looking for something
list(LENGTH UHD_BOOST_OPTIONAL_COMPONENTS UHD_BOOST_OPTIONAL_COMPONENTS_LEN)
list(LENGTH UHD_BOOST_REQUIRED_COMPONENTS UHD_BOOST_REQUIRED_COMPONENTS_LEN)
if(UHD_BOOST_OPTIONAL_COMPONENTS_LEN EQUAL 0 AND
   UHD_BOOST_REQUIRED_COMPONENTS_LEN EQUAL 0)
    # just see if Boost can be found
    find_package(Boost ${UHD_BOOST_MIN_VERSION} QUIET ${UHD_BOOST_REQUIRED})
    if(Boost_FOUND)
        message(STATUS "Boost version ${UHD_BOOST_MIN_VERSION} or greater - found")
    else()
        message(STATUS "Boost version ${UHD_BOOST_MIN_VERSION} or greater - not found")
    endif()
    return()
endif()

# if the OS is MINGW and if 'thread' is in the list, change its name
if(MINGW)
    list(FIND UHD_BOOST_REQUIRED_COMPONENTS "thread" THREAD_NDX)
    if(NOT ${THREAD_NDX} EQUAL -1)
        list(REMOVE_AT UHD_BOOST_REQUIRED_COMPONENTS ${THREAD_NDX})
        list(INSERT UHD_BOOST_REQUIRED_COMPONENTS ${THREAD_NDX} thread_win32)
    endif()
endif()

# if 'system' is in the list, make sure it comes last; sometimes this
# linkage makes a difference
list(FIND UHD_BOOST_REQUIRED_COMPONENTS "system" SYSTEM_NDX)
if(NOT ${SYSTEM_NDX} EQUAL -1)
    list(REMOVE_AT UHD_BOOST_REQUIRED_COMPONENTS ${SYSTEM_NDX})
    list(APPEND UHD_BOOST_REQUIRED_COMPONENTS "system")
endif()

# special library directory that's used by some Linux
if(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
endif(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")

# special Microsoft Visual C handling
if(MSVC)
    if(VCPKG_TARGET_TRIPLET)
        message(STATUS "  VCPKG Libs")
        string(FIND ${VCPKG_TARGET_TRIPLET} "static" vcpkg_check_static)
        string(FIND ${VCPKG_TARGET_TRIPLET} "static-md" vcpkg_check_static_md)
        if((NOT vcpkg_check_static EQUAL -1) AND vcpkg_check_static_md EQUAL -1)
            # Statically linked CRT
            message(STATUS "  VCPKG static CRT triplet found. Configuring compiler flags.")
            set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        endif((NOT vcpkg_check_static EQUAL -1) AND vcpkg_check_static_md EQUAL -1)
    else(VCPKG_TARGET_TRIPLET)
        set(BOOST_ALL_DYN_LINK "${BOOST_ALL_DYN_LINK}" CACHE BOOL "boost enable dynamic linking")
        if(BOOST_ALL_DYN_LINK)
            message(STATUS "  Dynamic Libs")
            add_definitions(-DBOOST_ALL_DYN_LINK) #setup boost auto-linking in msvc
        else(BOOST_ALL_DYN_LINK)
            message(STATUS "  Static Libs")
            set(UHD_BOOST_REQUIRED_COMPONENTS) #empty components list for static link
        endif(BOOST_ALL_DYN_LINK)
    endif(VCPKG_TARGET_TRIPLET)
endif(MSVC)

# Starting in CMake 3.15.0, if policy 'CMP0093' is available and set
# to 'NEW', then the Boost_VERSION will be returned as X.Y.Z (e.g.,
# 1.70.0) instead of XXYYZZ (107000). The OLD policy is the default
# for now, but is deprecated and relevant code should be updated to
# the NEW policy format. This change matches the version format
# returned by Boost's CMake scripts. See also:
# https://cmake.org/cmake/help/v3.15/policy/CMP0093.html#policy:CMP0093
# Tell FindBoost, if used, to report Boost_VERSION in X.Y.Z format.
# NOTE: This must be set -before- calling "find_package(Boost ...)".
# NOTE: This setting is for CMake scripts only; each language (e.g.,
# C++) that uses Boost maintains its current format for the Boost
# version variable.
if(POLICY CMP0093)
    cmake_policy(SET CMP0093 NEW)
endif()

# if no CXX STANDARD is set, default to that required by UHD: c++14
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
  message(WARNING "\nC++ standard not yet set; setting to C++14.\n")
endif()

# tell boost the linkage required
set(Boost_USE_STATIC_LIBS ${ENABLE_STATIC_LIBS})
# temporarily explicitly enable or disable shared libraries,
# build-wise; this can be disabled on a case by case basis for each
# library built.
if(BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS_SET TRUE)
    set(OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
else()
    set(BUILD_SHARED_LIBS_SET FALSE)
endif()
if(ENABLE_STATIC_LIBS)
    set(BUILD_SHARED_LIBS FALSE)
else()
    set(BUILD_SHARED_LIBS TRUE)
endif()

if(${UHD_BOOST_OPTIONAL_COMPONENTS_LEN} GREATER 0)
    message(STATUS "  Looking for optional Boost components...")
    find_package(Boost ${UHD_BOOST_MIN_VERSION} QUIET
        OPTIONAL_COMPONENTS ${UHD_BOOST_OPTIONAL_COMPONENTS})
endif()

if(${UHD_BOOST_REQUIRED_COMPONENTS_LEN} GREATER 0)
    message(STATUS "  Looking for required Boost components...")
    find_package(Boost ${UHD_BOOST_MIN_VERSION} QUIET
        COMPONENTS ${UHD_BOOST_REQUIRED_COMPONENTS} ${UHD_BOOST_REQUIRED})
endif()

# restore BUILD_SHARED_LIBS, if set
if(BUILD_SHARED_LIBS_SET)
    set(BUILD_SHARED_LIBS ${OLD_BUILD_SHARED_LIBS})
    unset(OLD_BUILD_SHARED_LIBS)
else()
    unset(BUILD_SHARED_LIBS)
endif()

if(NOT Boost_FOUND)
    # Boost was not found
    # clear out return variables, just in case
    unset(Boost_VERSION)
    unset(Boost_INCLUDE_DIRS)
    unset(Boost_LIBRARY_DIRS)
    unset(Boost_LIBRARIES)
else()
    # Boost was found; do fixups and related tests / checks

    # fix the Boost_VERSION to be X.Y.Z if CMake version < 3.15
    if(CMAKE_VERSION VERSION_LESS 3.15)
        set(Boost_VERSION "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")
    endif()

    # generic fix for some linking issues with Boost 1.68.0 or newer.
    if(NOT ${Boost_VERSION} VERSION_LESS 1.68.0)
        message(STATUS "  Enabling Boost Error Code Header Only")
        add_definitions(-DBOOST_ERROR_CODE_HEADER_ONLY)
    endif()

    # test for std::string_view in boost::asio only if we're using
    # c++17 or later and Boost 1.68.0 or later. The default is to not
    # use this feature. Boost 1.67.0 and earlier at best checked for
    # and used std::experimental::string_view, which causes issues for
    # some Boost & C++ versions ... so, just don't use it!
    set(USE_STD_STRING_VIEW_IN_BOOST_ASIO FALSE)
    if((NOT ${CMAKE_CXX_STANDARD} VERSION_LESS 17) AND
       (NOT ${Boost_VERSION} VERSION_LESS 1.68.0))
        # boost::asio::string_view can fail in compiling or linking simple
        # c++. This situation generally happens on for Boost 1.66 - 1.68
        # and macOS using Clang and -std=c++14 ... but it can occur in
        # other situations, so just do a generic check. This issue seems
        # to be fixed in Boost 1.70.0.
        message(STATUS "  Checking whether std::string_view works in boost::asio")
        # unset the return variable, otherwise the compile doesn't take place!
        unset(USE_STD_STRING_VIEW_IN_BOOST_ASIO)
        # set various CheckCXXSourceCompiles variables
        include(CheckCXXSourceCompiles)
        # FindBoost compatibility variables: Boost_INCLUDE_DIRS
        if(NOT Boost_INCLUDE_DIRS OR "${Boost_INCLUDE_DIRS}" STREQUAL "")
            get_target_property(Boost_INCLUDE_DIRS Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
        endif()
        # set the c++ standard to test using
        set(CMAKE_REQUIRED_FLAGS "-std=c++${CMAKE_CXX_STANDARD} -I${Boost_INCLUDE_DIRS}")
        # make this compile quite
        set(CMAKE_REQUIRED_QUIET TRUE)
        # disable Boost's use of std::experimental::string_view
        set(CMAKE_REQUIRED_DEFINITIONS -DBOOST_ASIO_DISABLE_STD_EXPERIMENTAL_STRING_VIEW)
        # Check if std::string_view works in boost::asio
        check_cxx_source_compiles(
            "#include <boost/asio/detail/string_view.hpp>
            int main()
            { boost::asio::string_view sv; }"
            USE_STD_STRING_VIEW_IN_BOOST_ASIO
        )
        # clear the various CheckCXXSourceCompiles variables
        unset(CMAKE_REQUIRED_FLAGS)
        unset(CMAKE_REQUIRED_QUIET)
        unset(CMAKE_REQUIRED_DEFINITIONS)
    endif()
    # use std::string_view in boost::asio?
    if(USE_STD_STRING_VIEW_IN_BOOST_ASIO)
        message(STATUS "    Enabling boost::asio use of std::string_view")
        add_definitions(-DBOOST_ASIO_HAS_STD_STRING_VIEW)
    else()
        message(STATUS "    Disabling boost::asio use of std::string_view")
        add_definitions(-DBOOST_ASIO_DISABLE_STD_STRING_VIEW)
    endif()
    unset(USE_STD_STRING_VIEW_IN_BOOST_ASIO)

    # disable Boost's use of std::experimental::string_view
    # works for Boost 1.67.0 and newer & doesn't hurt older
    add_definitions(-DBOOST_ASIO_DISABLE_STD_EXPERIMENTAL_STRING_VIEW)

    # Boost 1.70.0's find cmake scripts don't always set the expected
    # return variables. Replicate the commit that fixes that issue here:
    # https://github.com/boostorg/boost_install/commit/cfa8d55250dfc2635e907e42da423e4eb540dee5
    if(Boost_FOUND AND (${Boost_VERSION} VERSION_EQUAL 1.70.0))
        message(STATUS "  Enabling possible Boost 1.70.0 Fixes")

        # FindBoost compatibility variables: Boost_LIBRARIES, Boost_<C>_LIBRARY
        if(NOT Boost_LIBRARIES OR "${Boost_LIBRARIES}" STREQUAL "")
            set(Boost_LIBRARIES "")
            foreach(dep IN LISTS UHD_BOOST_REQUIRED_COMPONENTS UHD_BOOST_OPTIONAL_COMPONENTS)
                string(TOUPPER ${dep} _BOOST_DEP)
                if(NOT Boost_${_BOOST_DEP}_FOUND)
                    status(WARNING "  Boost component '${dep}' should have been found but somehow isn't listed as found. Ignoring and hoping for the best!")
                endif()
                list(APPEND Boost_LIBRARIES Boost::${dep})
                set(Boost_${_BOOST_DEP}_LIBRARY Boost::${dep})
            endforeach()
        endif()

        # FindBoost compatibility variables: Boost_INCLUDE_DIRS
        if(NOT Boost_INCLUDE_DIRS OR "${Boost_INCLUDE_DIRS}" STREQUAL "")
            get_target_property(Boost_INCLUDE_DIRS Boost::headers INTERFACE_INCLUDE_DIRECTORIES)
        endif()

        # FindBoost compatibility variables: Boost_LIBRARY_DIRS
        if(NOT Boost_LIBRARY_DIRS OR "${Boost_LIBRARY_DIRS}" STREQUAL "")
            set(Boost_LIBRARY_DIRS "")
            foreach(dep IN LISTS UHD_BOOST_REQUIRED_COMPONENTS UHD_BOOST_OPTIONAL_COMPONENTS)
                string(TOUPPER ${dep} _BOOST_DEP)
                if(NOT Boost_${_BOOST_DEP}_FOUND)
                    status(WARNING "  Boost component '${dep}' should have been found but somehow isn't listed as found. Ignoring and hoping for the best!")
                endif()
                if(Boost_USE_DEBUG_LIBS)
                    get_target_property(Boost_${dep}_LIBRARY Boost::${dep} IMPORTED_LOCATION_DEBUG)
                else()
                    get_target_property(Boost_${dep}_LIBRARY Boost::${dep} IMPORTED_LOCATION_RELEASE)
                endif()
                get_filename_component(Boost_${dep}_LIBRARY_DIR ${Boost_${dep}_LIBRARY} DIRECTORY ABSOLUTE)
                list(FIND Boost_LIBRARY_DIRS ${Boost_${dep}_LIBRARY_DIR} Boost_${dep}_LIBRARY_DIR_FOUND)
                if(${Boost_${dep}_LIBRARY_DIR_FOUND} EQUAL -1)
                    list(APPEND Boost_LIBRARY_DIRS ${Boost_${dep}_LIBRARY_DIR})
                endif()
            endforeach()
        endif()
        list(SORT Boost_LIBRARY_DIRS)
    endif()

    message(STATUS "  Boost version: ${Boost_VERSION}")
    message(STATUS "  Boost include directories: ${Boost_INCLUDE_DIRS}")
    message(STATUS "  Boost library directories: ${Boost_LIBRARY_DIRS}")
    message(STATUS "  Boost libraries: ${Boost_LIBRARIES}")
endif()

if(Boost_FOUND)
    message(STATUS "Looking for Boost version ${UHD_BOOST_MIN_VERSION} or greater - found")
else()
    message(STATUS "Looking for Boost version ${UHD_BOOST_MIN_VERSION} or greater - not found")
endif()

# unset some internal variables, if set
unset(Boost_LIBRARY_DIR)
unset(Boost_INCLUDE_DIR)

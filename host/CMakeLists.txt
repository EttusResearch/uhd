#
# Copyright 2010-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
#if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
#    message(FATAL_ERROR "Prevented in-tree built. This is bad practice.")
#endif(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})

########################################################################
# Project setup
########################################################################
cmake_minimum_required(VERSION 2.8.12)
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW) # Suppress Version warnings
endif(POLICY CMP0048)
project(UHD CXX C)
enable_testing()

#make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

########################################################################
# UHD Dependency Minimum Versions
########################################################################
# Full C++11 came with GCC 4.7, Clang >= 3.3.0, Apple Clang >= 500
set(GCC_MIN_VERSION "4.8.0")
set(CLANG_MIN_VERSION "3.3.0")
set(APPLECLANG_MIN_VERSION "500")
# Make sure to update the next two in unison:
set(MSVC_MIN_VERSION "1900")
set(MSVC_MIN_VERSION_READABLE "14.0")
# Other deps
set(BOOST_MIN_VERSION "1.53")

########################################################################
# Check Compiler Version
########################################################################
if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    if(DEFINED CMAKE_CXX_COMPILER_VERSION)
        if(${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS ${GCC_MIN_VERSION})
            message(WARNING "\nThe compiler selected to build UHD (GCC version ${CMAKE_CXX_COMPILER_VERSION} : ${CMAKE_CXX_COMPILER}) is older than that officially supported (${GCC_MIN_VERSION} minimum). This build may or not work. We highly recommend using a more recent GCC version.")
         endif()
    else()
        message(WARNING "\nCannot determine the version of the compiler selected to build UHD (GCC : ${CMAKE_CXX_COMPILER}). This build may or not work. We highly recommend using GCC version ${GCC_MIN_VERSION} or more recent.")
    endif()
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Og")
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    execute_process(COMMAND
        ${CMAKE_CXX_COMPILER} -v
        RESULT_VARIABLE res ERROR_VARIABLE err
        ERROR_STRIP_TRAILING_WHITESPACE)
    if(${res} STREQUAL "0")
        # output is in error stream
        string(REGEX MATCH "^Apple.*" IS_APPLE ${err})
        if("${IS_APPLE}" STREQUAL "")
            set(MIN_VERSION ${CLANG_MIN_VERSION})
            set(APPLE_STR "")
            # retrieve the compiler's version from it
            string(REGEX MATCH "clang version [0-9.]+" CLANG_OTHER_VERSION ${err})
            string(REGEX MATCH "[0-9.]+" CLANG_VERSION ${CLANG_OTHER_VERSION})
        else()
            set(MIN_VERSION ${APPLECLANG_MIN_VERSION})
            set(APPLE_STR "Apple ")
            # retrieve the compiler's version from it
            string(REGEX MATCH "(clang-[0-9.]+)" CLANG_APPLE_VERSION ${err})
            string(REGEX MATCH "[0-9.]+" CLANG_VERSION ${CLANG_APPLE_VERSION})
        endif()
        if(${CLANG_VERSION} VERSION_LESS "${MIN_VERSION}")
            message(WARNING "\nThe compiler selected to build UHD (${APPLE_STR}Clang version ${CLANG_VERSION} : ${CMAKE_CXX_COMPILER}) is older than that officially supported (${MIN_VERSION} minimum). This build may or not work. We highly recommend using Apple Clang version ${APPLECLANG_MIN_VERSION} or more recent, or Clang version ${CLANG_MIN_VERSION} or more recent.")
        endif()
    else()
        message(WARNING "\nCannot determine the version of the compiler selected to build UHD (${APPLE_STR}Clang : ${CMAKE_CXX_COMPILER}). This build may or not work. We highly recommend using Apple Clang version ${APPLECLANG_MIN_VERSION} or more recent, or Clang version ${CLANG_MIN_VERSION} or more recent.")
    endif()
elseif(MSVC)
    if(${MSVC_VERSION} VERSION_LESS ${MSVC_MIN_VERSION})
	message(FATAL_ERROR "\nMSVC version is less than the required minimum. Required: ${MSVC_MIN_VERSION_READABLE}")
    endif(${MSVC_VERSION} VERSION_LESS ${MSVC_MIN_VERSION})
endif()

if(CMAKE_VERSION VERSION_LESS "3.1")
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "--std=gnu++11 ${CMAKE_CXX_FLAGS}")
    elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
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

########################################################################
# Packaging Variables
########################################################################

set(LIBUHD_PKG ${LIBUHD_PKG} CACHE BOOL "Build Debian libuhd003 package")
set(LIBUHDDEV_PKG ${LIBUHDDEV_PKG} CACHE BOOL "Build Debian libuhd-dev package")
set(UHDHOST_PKG ${UHDPOST_PKG} CACHE BOOL "Build Debian uhd-host package")
include(UHDComponent) #enable components
include(UHDPackage)   #setup cpack

########################################################################
# Install Dirs
########################################################################
#when the library suffix should be 64 (applies to redhat linux family)
if(NOT DEFINED LIB_SUFFIX AND REDHAT AND CMAKE_SYSTEM_PROCESSOR MATCHES "64$")
    set(LIB_SUFFIX 64)
endif()
if(CMAKE_INSTALL_LIBDIR MATCHES lib64)
    set(LIB_SUFFIX 64)
endif()

set(LIB_SUFFIX ${LIB_SUFFIX} CACHE STRING "lib directory suffix")
set(RUNTIME_DIR bin)
set(LIBRARY_DIR lib${LIB_SUFFIX})
set(INCLUDE_DIR include)
set(PKG_DATA_DIR share/uhd)
if(NOT DEFINED PKG_LIB_DIR)
    set(PKG_LIB_DIR ${LIBRARY_DIR}/uhd)
endif()
set(PKG_DOC_DIR share/doc/uhd)
set(PKG_MAN_DIR share/man/man1)

########################################################################
# UHD config files
########################################################################
set(UHD_USER_CONF_FILE
    ".uhd/uhd.conf"
    CACHE STRING
    "Location of the user-specific UHD configuration file, relative to APPDATA (or HOME)"
)
if(UNIX)
    set(UHD_SYS_CONF_FILE
        "/etc/uhd/uhd.conf"
        CACHE STRING
        "Location of the system-wide UHD configuration file"
    )
elseif(WIN32)
    set(UHD_SYS_CONF_FILE
        "%programdata%/uhd/uhd.conf"
        CACHE STRING
        "Location of the system-wide UHD configuration file"
    )
else()
    message(WARNING "Not setting UHD_SYS_CONF_FILE!")
endif()

########################################################################
# UHD Image Directories
########################################################################
if(NOT DEFINED UHD_IMAGES_DIR)
    if(DEFINED FPGA_IMAGES_DIR)
        set(UHD_IMAGES_DIR ${FPGA_IMAGES_DIR})
    else(DEFINED FPGA_IMAGES_DIR)
        file(TO_NATIVE_PATH ${CMAKE_INSTALL_PREFIX}/share/uhd/images using_images_dir)
        set(UHD_IMAGES_DIR ${using_images_dir})
    endif(DEFINED FPGA_IMAGES_DIR)
endif(NOT DEFINED UHD_IMAGES_DIR)

message( STATUS "Using UHD Images Directory: ${UHD_IMAGES_DIR}" )

if(DEFINED UHD_IMAGES_DIR_WINREG_KEY)
    add_definitions(-DUHD_IMAGES_DIR_WINREG_KEY=${UHD_IMAGES_DIR_WINREG_KEY})
endif(DEFINED UHD_IMAGES_DIR_WINREG_KEY)

########################################################################
# Local Include Dir
########################################################################
include_directories(${CMAKE_BINARY_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include)

########################################################################
# Static Lib Configuration
########################################################################
option(ENABLE_STATIC_LIBS "Enable building of static libraries" OFF)
if(ENABLE_STATIC_LIBS)
    message(STATUS "Building Static Libraries: ${ENABLE_STATIC_LIBS}")
endif(ENABLE_STATIC_LIBS)

########################################################################
# On Apple only, set install name and use rpath correctly, if not already set
########################################################################
if(APPLE)
    if(NOT CMAKE_INSTALL_NAME_DIR)
        set(CMAKE_INSTALL_NAME_DIR
            ${CMAKE_INSTALL_PREFIX}/${LIBRARY_DIR} CACHE
            PATH "Library Install Name Destination Directory" FORCE)
    endif(NOT CMAKE_INSTALL_NAME_DIR)
    if(NOT CMAKE_INSTALL_RPATH)
        set(CMAKE_INSTALL_RPATH
            ${CMAKE_INSTALL_PREFIX}/${LIBRARY_DIR} CACHE
            PATH "Library Install RPath" FORCE)
    endif(NOT CMAKE_INSTALL_RPATH)
    if(NOT CMAKE_BUILD_WITH_INSTALL_RPATH)
        set(CMAKE_BUILD_WITH_INSTALL_RPATH ON CACHE
            BOOL "Do Build Using Library Install RPath" FORCE)
    endif(NOT CMAKE_BUILD_WITH_INSTALL_RPATH)
endif(APPLE)

########################################################################
# Optional Compiler Flags
########################################################################
include(CheckCXXCompilerFlag)
macro(UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG flag have)
    CHECK_CXX_COMPILER_FLAG(${flag} ${have})
    if(${have})
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
    endif(${have})
endmacro(UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG)

#select the release build type by default to get optimization flags
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE "Release")
   message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}" CACHE STRING "")

#force UHD_RELEASE_MODE to be a string for cmake-gui
set(UHD_RELEASE_MODE "${UHD_RELEASE_MODE}" CACHE STRING "UHD Release Mode")

if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" OR
    ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    if(STRIP_BINARIES)
        if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s")
        endif(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    endif(STRIP_BINARIES)
    add_definitions(-Wall)
    add_definitions(-Wextra)
    add_definitions(-Wsign-compare)
    #add_definitions(-Wconversion)
    #add_definitions(-pedantic)
    #add_definitions(-ansi)
    if(NOT WIN32)
        #only export symbols that are declared to be part of the uhd api (non dll platforms)
        UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG(-fvisibility=hidden HAVE_VISIBILITY_HIDDEN)
        UHD_ADD_OPTIONAL_CXX_COMPILER_FLAG(-fvisibility-inlines-hidden HAVE_VISIBILITY_INLINES_HIDDEN)
    endif(NOT WIN32)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Coverage")
        include(CodeCoverage)
        setup_target_for_coverage(coverage "ctest || return 0" coverage) # never fail ctest, always generate coverage report
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -pthread -g -O0 -fprofile-arcs -ftest-coverage" CACHE STRING "Flags used by the C++ compiler during Coverage builds." FORCE)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -pthread -g -O0 -fprofile-arcs -ftest-coverage" CACHE STRING "Flags used by the C compiler during Coverage builds." FORCE)
    endif()
endif()

if(MSVC)
    include_directories(${CMAKE_SOURCE_DIR}/cmake/msvc)
    add_definitions( #stop all kinds of compatibility warnings
        -DWIN32_LEAN_AND_MEAN
        -DVC_EXTRALEAN
        -D_SCL_SECURE_NO_WARNINGS
        -D_SCL_SECURE_NO_DEPRECATE
        -D_CRT_SECURE_NO_WARNINGS
        -D_CRT_SECURE_NO_DEPRECATE
        -D_CRT_NONSTDC_NO_WARNINGS
        -D_CRT_NONSTDC_NO_DEPRECATE
        -D_WINSOCK_DEPRECATED_NO_WARNINGS
    )
    # multi-threaded build and increases the number of addressable sections in an .obj file.
    add_compile_options(/MP /bigobj)
endif(MSVC)

if(CYGWIN)
    add_definitions(-D__USE_W32_SOCKETS) #boost asio says we need this
endif(CYGWIN)

if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0501) #minimum version required is windows xp
    add_definitions(-DNOMINMAX) #disables stupidity and enables std::min and std::max
endif(WIN32)

########################################################################
# Choose a version of Python
########################################################################
option(ENABLE_PYTHON3 "Enable Python 3. Default is Python 2" OFF)
set(BOOST_PYTHON_COMPONENT python)
set(BOOST_PYTHON_VERSION 2.7)
if(${ENABLE_PYTHON3})
    set(BOOST_PYTHON_COMPONENT python3)
    set(BOOST_PYTHON_VERSION 3)
endif()

########################################################################
# Setup Boost
########################################################################
message(STATUS "")
message(STATUS "Configuring Boost C++ Libraries...")
set(BOOST_REQUIRED_COMPONENTS
    chrono
    date_time
    filesystem
    program_options
    regex
    system
    unit_test_framework
    serialization
)

set(BOOST_OPTIONAL_COMPONENTS ${BOOST_PYTHON_COMPONENT})
if(MINGW)
    list(APPEND BOOST_REQUIRED_COMPONENTS thread_win32)
else()
    list(APPEND BOOST_REQUIRED_COMPONENTS thread)
endif()

if(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")
    list(APPEND BOOST_LIBRARYDIR "/usr/lib64") #fedora 64-bit fix
endif(UNIX AND NOT BOOST_ROOT AND EXISTS "/usr/lib64")

if(MSVC)
    set(BOOST_ALL_DYN_LINK "${BOOST_ALL_DYN_LINK}" CACHE BOOL "boost enable dynamic linking")
    if(BOOST_ALL_DYN_LINK)
        add_definitions(-DBOOST_ALL_DYN_LINK) #setup boost auto-linking in msvc
    else(BOOST_ALL_DYN_LINK)
        set(BOOST_REQUIRED_COMPONENTS) #empty components list for static link
    endif(BOOST_ALL_DYN_LINK)
endif(MSVC)

set(Boost_ADDITIONAL_VERSIONS
    "1.46.0" "1.46" "1.47.0" "1.47" "1.48.0" "1.48" "1.48.0" "1.49" "1.50.0" "1.50"
    "1.51.0" "1.51" "1.52.0" "1.52" "1.53.0" "1.53" "1.54.0" "1.54" "1.55.0" "1.55"
    "1.56.0" "1.56" "1.57" "1.57" "1.58" "1.59" "1.60" "1.61" "1.62" "1.63"
    "1.64" "1.65" "1.66" "1.67" "1.68" "1.69"
)

#Python API requirements
message(STATUS "Looking for optional Boost components...")
find_package(Boost ${BOOST_MIN_VERSION} OPTIONAL_COMPONENTS ${BOOST_OPTIONAL_COMPONENTS})

message(STATUS "Looking for required Boost components...")
find_package(Boost ${BOOST_MIN_VERSION} COMPONENTS ${BOOST_REQUIRED_COMPONENTS})

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

#Check to see if the Python version we're looking for exists
set(BOOST_PYTHON_FOUND ${Boost_PYTHON_FOUND})
set(BOOST_PYTHON_LIBRARY ${Boost_PYTHON_LIBRARY})
if(${ENABLE_PYTHON3})
    set(BOOST_PYTHON_FOUND ${Boost_PYTHON3_FOUND})
    set(BOOST_PYTHON_LIBRARY ${Boost_PYTHON3_LIBRARY})
endif()

message(STATUS "Boost include directories: ${Boost_INCLUDE_DIRS}")
message(STATUS "Boost library directories: ${Boost_LIBRARY_DIRS}")
message(STATUS "Boost libraries: ${Boost_LIBRARIES}")

########################################################################
# Additional settings for build environment
########################################################################
# Note: RFNoC never gets fully disabled, but the public APIs do
set(ENABLE_RFNOC OFF CACHE BOOL "Export RFNoC includes and symbols")
include(UHDGlobalDefs)
include(UHDLog)

########################################################################
# Check Python Modules
########################################################################
find_package(PythonInterp ${BOOST_PYTHON_VERSION})
find_package(PythonLibs ${BOOST_PYTHON_VERSION})
include(UHDPython)

set(PYTHON_VERSION "platform.python_version() >= '2.7' and platform.python_version() < '3.0'")
if(${ENABLE_PYTHON3})
    set(PYTHON_VERSION "platform.python_version() >= '3.0'")
endif()

PYTHON_CHECK_MODULE(
    "Python version ${BOOST_PYTHON_VERSION} or greater"
    "platform" ${PYTHON_VERSION}
    HAVE_PYTHON_PLAT_MIN_VERSION
)

PYTHON_CHECK_MODULE(
    "Mako templates 0.4.2 or greater"
    "mako" "mako.__version__ >= '0.4.2'"
    HAVE_PYTHON_MODULE_MAKO
)

PYTHON_CHECK_MODULE(
    "requests 2.0 or greater"
    "requests" "requests.__version__ >= '2.0'"
    HAVE_PYTHON_MODULE_REQUESTS
)

PYTHON_CHECK_MODULE(
    "numpy 1.7 or greater"
    "numpy" "LooseVersion(numpy.__version__) >= LooseVersion('1.7')"
    HAVE_PYTHON_MODULE_NUMPY
)

set(PYTHON_ADDITIONAL_VERSIONS 2.7 3.4 3.5)

########################################################################
# Create Uninstall Target
########################################################################
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)

add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

########################################################################
# Install Package Docs
########################################################################
UHD_INSTALL(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    DESTINATION ${PKG_DOC_DIR}
    COMPONENT readme
)

########################################################################
# Register top level components
########################################################################
LIBUHD_REGISTER_COMPONENT("LibUHD" ENABLE_LIBUHD ON "Boost_FOUND;HAVE_PYTHON_PLAT_MIN_VERSION;HAVE_PYTHON_MODULE_MAKO" OFF ON)
LIBUHD_REGISTER_COMPONENT("LibUHD - C API" ENABLE_C_API ON "ENABLE_LIBUHD" OFF OFF)
if(WIN32)
    LIBUHD_REGISTER_COMPONENT("LibUHD - Python API" ENABLE_PYTHON_API OFF "ENABLE_LIBUHD;BOOST_PYTHON_FOUND;HAVE_PYTHON_MODULE_NUMPY;PythonLibs_FOUND" OFF OFF)
else()
    LIBUHD_REGISTER_COMPONENT("LibUHD - Python API" ENABLE_PYTHON_API ON "ENABLE_LIBUHD;BOOST_PYTHON_FOUND;HAVE_PYTHON_MODULE_NUMPY;PythonLibs_FOUND" OFF OFF)
endif(WIN32)
LIBUHD_REGISTER_COMPONENT("Examples" ENABLE_EXAMPLES ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("Utils" ENABLE_UTILS ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("Tests" ENABLE_TESTS ON "ENABLE_LIBUHD" OFF OFF)


########################################################################
# Check for fpga-src submodule
########################################################################
set(HAS_FPGA_SUBMODULE FALSE)
execute_process(
    COMMAND ${PYTHON_EXECUTABLE} -c "import os; print(os.path.abspath(os.path.join('${CMAKE_SOURCE_DIR}', '..', 'fpga-src')))"
    OUTPUT_VARIABLE FPGA_SUBMODULE_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
if(EXISTS "${FPGA_SUBMODULE_DIR}/docs/fpga.md")
    set(HAS_FPGA_SUBMODULE TRUE)
    message(STATUS "")
    message(STATUS "Found FPGA submodule: ${FPGA_SUBMODULE_DIR}")
endif(EXISTS "${FPGA_SUBMODULE_DIR}/docs/fpga.md")

########################################################################
# Add the subdirectories
########################################################################
if(ENABLE_LIBUHD)
    add_subdirectory(lib)
endif(ENABLE_LIBUHD)

add_subdirectory(include)

if(ENABLE_EXAMPLES)
    add_subdirectory(examples)
endif(ENABLE_EXAMPLES)

if(ENABLE_TESTS)
    add_subdirectory(tests)
endif(ENABLE_TESTS)

if(ENABLE_UTILS)
    add_subdirectory(utils)
endif(ENABLE_UTILS)

add_subdirectory(docs)

if(ENABLE_PYTHON_API)
    add_subdirectory(python)
endif()

########################################################################
# Create Pkg Config File
########################################################################
foreach(inc ${Boost_INCLUDE_DIRS})
    list(APPEND UHD_PC_CFLAGS "-I${inc}")
endforeach(inc)

foreach(lib ${Boost_LIBRARY_DIRS})
    list(APPEND UHD_PC_LIBS "-L${lib}")
endforeach(lib)

#use space-separation format for the pc file
string(REPLACE ";" " " UHD_PC_REQUIRES "${UHD_PC_REQUIRES}")
string(REPLACE ";" " " UHD_PC_CFLAGS "${UHD_PC_CFLAGS}")
string(REPLACE ";" " " UHD_PC_LIBS "${UHD_PC_LIBS}")

#unset these vars to avoid hard-coded paths to cross environment
if(CMAKE_CROSSCOMPILING)
    set(UHD_PC_CFLAGS)
    set(UHD_PC_LIBS)
endif(CMAKE_CROSSCOMPILING)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/uhd.pc.in
    ${CMAKE_CURRENT_BINARY_DIR}/uhd.pc
@ONLY)

UHD_INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/uhd.pc
    DESTINATION ${LIBRARY_DIR}/pkgconfig
    COMPONENT "devel"
)

########################################################################
# Create and Install UHD cmake files
########################################################################
if(NOT CMAKE_MODULES_DIR)
   set(CMAKE_MODULES_DIR lib${LIB_SUFFIX}/cmake)
endif(NOT CMAKE_MODULES_DIR)

# UHDConfig.cmake needs UHD_LINK_LIST_STATIC set:
list(APPEND UHD_LINK_LIST_STATIC "dl")
list(APPEND UHD_LINK_LIST_STATIC "pthread")
foreach(Boost_Comp ${BOOST_REQUIRED_COMPONENTS})
    if(NOT ${Boost_Comp} STREQUAL "unit_test_framework")
        list(APPEND UHD_LINK_LIST_STATIC "boost_${Boost_Comp}")
    endif(NOT ${Boost_Comp} STREQUAL "unit_test_framework")
endforeach(Boost_Comp)
if(ENABLE_USB)
    list(APPEND UHD_LINK_LIST_STATIC "usb-1.0")
endif(ENABLE_USB)
# UHDConfig.cmake also needs UHD_RFNOC_FOUND
if(ENABLE_RFNOC)
    set(UHD_RFNOC_FOUND "TRUE")
else()
    set(UHD_RFNOC_FOUND "FALSE")
endif(ENABLE_RFNOC)

configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/Modules/UHDConfigVersion.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/Modules/UHDConfigVersion.cmake
    @ONLY
)
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/Modules/UHDConfig.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/Modules/UHDConfig.cmake
    @ONLY
)

set(cmake_configs
    ${CMAKE_BINARY_DIR}/cmake/Modules/UHDConfig.cmake
    ${CMAKE_BINARY_DIR}/cmake/Modules/UHDConfigVersion.cmake
)

UHD_INSTALL(
    FILES ${cmake_configs}
    DESTINATION ${CMAKE_MODULES_DIR}/uhd
    COMPONENT "devel"
)

########################################################################
# Handle pre-built UHD Images for packaging
########################################################################
if(DEFINED UHD_IMAGES_SRC_DIR AND EXISTS "${UHD_IMAGES_SRC_DIR}")
    message(STATUS "Reading images from directory `${UHD_IMAGES_SRC_DIR}'")
    file(GLOB_RECURSE _image_files "${UHD_IMAGES_SRC_DIR}/*")
    message(STATUS "These images files will be installed/packaged:")
    foreach(_img ${_image_files})
        message(STATUS "  ${_img}")
    endforeach(_img)
    UHD_INSTALL(FILES ${_image_files} DESTINATION ${PKG_DATA_DIR}/images COMPONENT images)
endif(DEFINED UHD_IMAGES_SRC_DIR AND EXISTS "${UHD_IMAGES_SRC_DIR}")

########################################################################
# Print Summary
########################################################################
if(LIBUHD_PKG)
    message(STATUS "")
    set(PRINT_APPEND " (Debian libuhd003 package configuration)")
elseif(LIBUHDDEV_PKG)
    message(STATUS "")
    set(PRINT_APPEND " (Debian libuhd-dev package configuration)")
elseif(UHDHOST_PKG)
    message(STATUS "")
    set(PRINT_APPEND " (Debian uhd-host package configuration)")
endif(LIBUHD_PKG)
UHD_PRINT_COMPONENT_SUMMARY()
if(UHD_VERSION_DEVEL AND NOT UHD_GIT_BRANCH MATCHES "^UHD-")
    message(STATUS "******************************************************")
    if(UHD_GIT_BRANCH STREQUAL "master")
        message(STATUS "* You are building the UHD development master branch.")
        message(STATUS "* For production code, we recommend our stable,")
        message(STATUS "* releases or using the release branch (maint).")
    else()
        message(STATUS "* You are building a development branch of UHD.")
        message(STATUS "* These branches are designed to provide early access")
        message(STATUS "* to UHD and USRP features, but should be considered")
        message(STATUS "* unstable and/or experimental!")
    endif(UHD_GIT_BRANCH STREQUAL "master")
    message(STATUS "******************************************************")
endif(UHD_VERSION_DEVEL AND NOT UHD_GIT_BRANCH MATCHES "^UHD-")
message(STATUS "Building version: ${UHD_VERSION}${PRINT_APPEND}")
message(STATUS "Using install prefix: ${CMAKE_INSTALL_PREFIX}")

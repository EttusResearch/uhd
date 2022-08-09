#
# Copyright 2010-2018 Ettus Research, a National Instruments Company
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Project setup
########################################################################
# NOTE! If you change the version in the following line, also change
# CMAKE_MIN_VERSION below. (This won't take a variable).
cmake_minimum_required(VERSION 3.8)
if(POLICY CMP0048)
  cmake_policy(SET CMP0048 NEW) # Suppress Version warnings
endif(POLICY CMP0048)
project(UHD CXX C)
enable_testing()

#make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${UHD_SOURCE_DIR}/cmake/Modules)

########################################################################
# UHD Dependency Minimum Versions
########################################################################
set(CMAKE_MIN_VERSION "3.8")
set(GCC_MIN_VERSION "6.3.0")
set(CLANG_MIN_VERSION "6.0.0")
set(APPLECLANG_MIN_VERSION "800")
# Make sure to update the next two in unison:
set(MSVC_MIN_VERSION "1910")
set(MSVC_MIN_VERSION_READABLE "15.0")
# This Python version gets used for Python API (if requested) as well as
# all the build-time Python scripts
set(PYTHON_MIN_VERSION "3.6")
# Other deps
set(BOOST_MIN_VERSION "1.65")
set(NUMPY_MIN_VERSION "1.11")
set(RUAMEL.YAML_MIN_VERSION "0.15")
set(PY_MAKO_MIN_VERSION "0.4.2")
set(PY_REQUESTS_MIN_VERSION "2.0")

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

set(CMAKE_CXX_STANDARD 14)

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
if(NOT DEFINED PKG_DOC_DIR)
    set(PKG_DOC_DIR share/doc/uhd)
endif()
set(PKG_MAN_DIR share/man/man1)

########################################################################
# UHD config files
########################################################################
set(UHD_USER_CONF_FILE
    "uhd.conf"
    CACHE STRING
    "Location of the user-specific UHD configuration file, relative to APPDATA (or XDG_CONFIG_HOME)"
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
include_directories(${UHD_BINARY_DIR}/include)
include_directories(${UHD_SOURCE_DIR}/include)

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
    include_directories(${UHD_SOURCE_DIR}/cmake/msvc)
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
# Setup Boost
########################################################################
message(STATUS "")
message(STATUS "Configuring Boost C++ Libraries...")
# NO optional Boost components!
# ALL required Boost components!
set(UHD_BOOST_REQUIRED_COMPONENTS
    chrono
    date_time
    filesystem
    program_options
    system
    serialization
    thread
    unit_test_framework
)

include(UHDBoost)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

########################################################################
# Additional settings for build environment
########################################################################
include(UHDGlobalDefs)
include(UHDLog)

########################################################################
# Check Python Modules
########################################################################
include(UHDPython)

PYTHON_CHECK_MODULE(
    "Python version ${PYTHON_MIN_VERSION} or greater"
    "platform" "LooseVersion(platform.python_version()) >= LooseVersion('${PYTHON_MIN_VERSION}')"
    HAVE_PYTHON_PLAT_MIN_VERSION
)

PYTHON_CHECK_MODULE(
    "Mako templates ${PY_MAKO_MIN_VERSION} or greater"
    "mako" "LooseVersion(mako.__version__) >= LooseVersion('${PY_MAKO_MIN_VERSION}')"
    HAVE_PYTHON_MODULE_MAKO
)

PYTHON_CHECK_MODULE(
    "requests ${PY_REQUESTS_MIN_VERSION} or greater"
    "requests" "LooseVersion(requests.__version__) >= LooseVersion('${PY_REQUESTS_MIN_VERSION}')"
    HAVE_PYTHON_MODULE_REQUESTS
)

PYTHON_CHECK_MODULE(
    "numpy ${NUMPY_MIN_VERSION} or greater"
    "numpy" "LooseVersion(numpy.__version__) >= LooseVersion('${NUMPY_MIN_VERSION}')"
    HAVE_PYTHON_MODULE_NUMPY
)

PYTHON_CHECK_MODULE(
    "ruamel.yaml ${RUAMEL.YAML_MIN_VERSION} or greater"
    "ruamel.yaml" "LooseVersion(ruamel.yaml.__version__) >= LooseVersion('${RUAMEL.YAML_MIN_VERSION}')"
    HAVE_PYTHON_MODULE_YAML
)

########################################################################
# Option to use QEMU for running unittests
#
# This is useful when cross-compiling UHD for other architectures like arm or
# aarch64
#
# QEMU_EXECUTABLE must be set to the name of the QEMU executable. When using
# openembedded build environment, this should be set to "qemu-${TUNE_ARCH}"
# or "${WORKDIR}/recipe-sysroot-native/usr/bin/qemu-${TUNE_ARCH}"
#
# QEMU_SYSROOT must be set to the same sysroot of the target architecture that
# was used when compiling UHD. When using openembedded build environment, this
# should be set to "${WORKDIR}/recipe-sysroot".
#
# QEMU_PYTHON_EXECUTABLE must be set to the path of the python executable
# compiled for the target architecture. If it is included in the target sysroot,
# this is typically ${QEMU_SYSROOT}${PYTHON_RUNTIME_EXECUTABLE}. The variable
# is automatically set to this path if the executable is available.
########################################################################
option(ENABLE_QEMU_UNITTESTS "Use QEMU for running unittests" OFF)
mark_as_advanced(ENABLE_QEMU_UNITTESTS)
if(ENABLE_QEMU_UNITTESTS)
    message(STATUS "")
    message(STATUS "Use QEMU for running unittests: ${ENABLE_QEMU_UNITTESTS}")
    if("${QEMU_EXECUTABLE}" STREQUAL "")
      if(NOT "$ENV{OECORE_NATIVE_SYSROOT}" STREQUAL "" AND
         NOT "$ENV{OECORE_TARGET_ARCH}" STREQUAL "" AND
         EXISTS "$ENV{OECORE_NATIVE_SYSROOT}/usr/bin/qemu-$ENV{OECORE_TARGET_ARCH}")
         set(QEMU_EXECUTABLE "$ENV{OECORE_NATIVE_SYSROOT}/usr/bin/qemu-$ENV{OECORE_TARGET_ARCH}")
      else()
        message(FATAL_ERROR "The variable QEMU_EXECUTABLE is not set")
      endif()
    endif()
    if("${QEMU_SYSROOT}" STREQUAL "")
      if(NOT "$ENV{OECORE_TARGET_SYSROOT}" STREQUAL "" AND
         EXISTS "$ENV{OECORE_TARGET_SYSROOT}")
         set(QEMU_SYSROOT, "$ENV{OECORE_TARGET_SYSROOT}")
      else()
        message(FATAL_ERROR "The variable QEMU_SYSROOT is not set")
      endif()
    endif()
    if("${QEMU_PYTHON_EXECUTABLE}" STREQUAL "")
      if (EXISTS ${QEMU_SYSROOT}${RUNTIME_PYTHON_EXECUTABLE})
        set(QEMU_PYTHON_EXECUTABLE ${QEMU_SYSROOT}${RUNTIME_PYTHON_EXECUTABLE})
      else()
        message(FATAL_ERROR "The variable QEMU_PYTHON_EXECUTABLE is not set")
      endif()
    endif()
    message(STATUS "  QEMU executable: ${QEMU_EXECUTABLE}")
    message(STATUS "  QEMU Python executable: ${QEMU_PYTHON_EXECUTABLE}")
    message(STATUS "  QEMU sysroot: ${QEMU_SYSROOT}")
endif(ENABLE_QEMU_UNITTESTS)

########################################################################
# Create Uninstall Target
########################################################################
configure_file(
    ${UHD_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
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
LIBUHD_REGISTER_COMPONENT("LibUHD" ENABLE_LIBUHD ON "Boost_FOUND;HAVE_PYTHON_MODULE_MAKO" OFF ON)
LIBUHD_REGISTER_COMPONENT("LibUHD - C API" ENABLE_C_API ON "ENABLE_LIBUHD" OFF OFF)
if(WIN32)
  LIBUHD_REGISTER_COMPONENT("LibUHD - Python API" ENABLE_PYTHON_API OFF "ENABLE_LIBUHD;HAVE_PYTHON_MODULE_NUMPY;HAVE_PYTHON_LIBS" OFF OFF)
else()
  LIBUHD_REGISTER_COMPONENT("LibUHD - Python API" ENABLE_PYTHON_API ON "ENABLE_LIBUHD;HAVE_PYTHON_MODULE_NUMPY;HAVE_PYTHON_LIBS" OFF OFF)
endif(WIN32)
LIBUHD_REGISTER_COMPONENT("Examples" ENABLE_EXAMPLES ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("Utils" ENABLE_UTILS ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("Tests" ENABLE_TESTS ON "ENABLE_LIBUHD" OFF OFF)

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
foreach(Boost_Comp ${UHD_BOOST_REQUIRED_COMPONENTS})
    if(NOT ${Boost_Comp} STREQUAL "unit_test_framework")
        list(APPEND UHD_LINK_LIST_STATIC "boost_${Boost_Comp}")
    endif(NOT ${Boost_Comp} STREQUAL "unit_test_framework")
endforeach(Boost_Comp)
if(ENABLE_USB)
    list(APPEND UHD_LINK_LIST_STATIC "usb-1.0")
endif(ENABLE_USB)
set(UHD_RFNOC_FOUND "TRUE")

configure_file(
    ${UHD_SOURCE_DIR}/cmake/Modules/UHDConfigVersion.cmake.in
    ${UHD_BINARY_DIR}/cmake/Modules/UHDConfigVersion.cmake
    @ONLY
)
configure_file(
    ${UHD_SOURCE_DIR}/cmake/Modules/UHDConfig.cmake.in
    ${UHD_BINARY_DIR}/cmake/Modules/UHDConfig.cmake
    @ONLY
)

set(uhd_cmake_scripts
    ${UHD_BINARY_DIR}/cmake/Modules/UHDConfig.cmake
    ${UHD_BINARY_DIR}/cmake/Modules/UHDConfigVersion.cmake
    ${UHD_SOURCE_DIR}/cmake/Modules/UHDBoost.cmake
)

UHD_INSTALL(
    FILES ${uhd_cmake_scripts}
    DESTINATION ${CMAKE_MODULES_DIR}/uhd
    COMPONENT "devel"
)

########################################################################
# Handle pre-built UHD Images for packaging
########################################################################
if(DEFINED UHD_IMAGES_SRC_DIR AND EXISTS "${UHD_IMAGES_SRC_DIR}")
    message(STATUS "Reading images from directory `${UHD_IMAGES_SRC_DIR}'")
    file(GLOB_RECURSE _image_files RELATIVE "${UHD_SOURCE_DIR}" "${UHD_IMAGES_SRC_DIR}/*")
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

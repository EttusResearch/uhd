#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

cmake_minimum_required(VERSION 3.12)
project(rfnoc-gain CXX C)

#make sure our local CMake Modules path comes first
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_SOURCE_DIR}/cmake/Modules)

# Set the version information here
set(VERSION_MAJOR 4)
set(VERSION_API   9)
set(VERSION_ABI   0)

########################################################################
# Setup install directories
########################################################################
set(RFNOC_DATA_DIR     share
    CACHE PATH "Base location for data")
set(UHD_PKG_DATA_DIR ${RFNOC_DATA_DIR}/uhd/
    CACHE PATH "Path to install UHD-related package data")
set(RFNOC_PKG_DATA_DIR ${RFNOC_DATA_DIR}/uhd/rfnoc/
    CACHE PATH "Path to install RFNoC package data")
set(GATEWARE_PKG_DATA_DIR ${RFNOC_PKG_DATA_DIR}/fpga/gain
    CACHE PATH "Path to install RFNoC package data")
set(PROJECT_DATA_DIR   ${RFNOC_PKG_DATA_DIR}/gain/
    CACHE PATH "Path for this project's package data")
set(MODULES_D_DIR   ${UHD_PKG_DATA_DIR}/modules.d/
    CACHE PATH "Path for the UHD modules.d/ directory")

if(NOT DEFINED LIB_SUFFIX AND REDHAT AND CMAKE_SYSTEM_PROCESSOR MATCHES "64$")
    set(LIB_SUFFIX 64)
endif()
if(CMAKE_INSTALL_LIBDIR MATCHES lib64)
    set(LIB_SUFFIX 64)
endif()

########################################################################
# Find bash (for executing make and sourcing the Vivado env)
########################################################################
find_program(BASH "bash")
if(BASH)
    message(STATUS "Found bash interpreter: ${BASH}")
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/Modules/run_testbench.sh.in
        ${CMAKE_BINARY_DIR}/cmake/Modules/run_testbench.sh
        @ONLY
    )
else()
    message(WARNING
        "Bash interpreter not found: Cannot generate FPGA targets.")
endif()

###########################################################################
# Find UHD
###########################################################################
find_package(UHD 4.7)
if(UHD_FOUND)
    message(STATUS "Found UHD:")
    message(STATUS " * Version: ${UHD_VERSION}")
    include_directories(${UHD_INCLUDE_DIRS})
    message(STATUS " * INCLUDES = ${UHD_INCLUDE_DIRS}")
    link_directories(${UHD_LIBRARIES})
    message(STATUS " * LIBS = ${UHD_LIBRARIES}")
    find_program(_rfnoc_image_builder_exe
        "rfnoc_image_builder"
    )
    if (_rfnoc_image_builder_exe)
        message(STATUS
            " * rfnoc_image_builder = ${_rfnoc_image_builder_exe}")
    endif()
else()
    message(WARNING "UHD not found. Cannot build block controllers.")
endif()

###########################################################################
# Find Python and uhd Python module
###########################################################################
include(UHDPython)

UHD_PYTHON_CHECK_MODULE(
    "UHD Python API"
    "uhd"
    "uhd.__version__ == '${UHD_VERSION}'"
    HAVE_PYTHON_MODULE_UHD
)

if(pybind11_FOUND AND HAVE_PYTHON_MODULE_UHD)
    set(ENABLE_PYTHON_API TRUE
        CACHE BOOL "Enable Python API")
else()
    set(ENABLE_PYTHON_API FALSE
        CACHE BOOL "Enable Python API")
endif()

if(ENABLE_PYTHON_API)
    message(STATUS "Enabling Python API for this module.")
endif()

###########################################################################
# Find FPGA
###########################################################################
set(UHD_FPGA_DIR "" CACHE PATH "Path to FPGA source directory")
message(STATUS "Checking FPGA source directory...")
if(NOT UHD_FPGA_DIR)
    message(WARNING
        "Could not find FPGA directory. Skipping all FPGA targets. "
        "Please provide it using -DUHD_FPGA_DIR!")
endif(NOT UHD_FPGA_DIR)
if(UHD_FPGA_DIR AND NOT EXISTS ${UHD_FPGA_DIR}/usrp3/top/Makefile.common)
    message(
        FATAL_ERROR
        "Invalid FPGA source directory: ${UHD_FPGA_DIR}. "
        "Please provide it using -DUHD_FPGA_DIR!")
endif()
message(STATUS "Using FPGA source directory: ${UHD_FPGA_DIR}")

set(UHD_FPGA_DEFAULT_DEVICE "x310"
    CACHE STRING "Default device for testbench execution")

set(FPGA_SIM_CMD "xsim"
    CACHE STRING "Executable for running testbenches (e.g., xsim, vsim)")

########################################################################
# Testbench targets and FPGA helpers
########################################################################
add_custom_target(testbenches)
macro(RFNOC_ADD_TB_DIR)
    if(BASH AND UHD_FPGA_DIR)
        get_filename_component(_tb_dir "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
        set(_target_name "${_tb_dir}_tb")
        message(STATUS "Adding testbench target: ${_target_name}")
        add_custom_target(${_target_name}
            COMMAND ${BASH} ${CMAKE_BINARY_DIR}/cmake/Modules/run_testbench.sh ${UHD_FPGA_DIR} ${UHD_FPGA_DEFAULT_DEVICE} ${CMAKE_CURRENT_SOURCE_DIR} ${FPGA_SIM_CMD}
        )
        add_dependencies(testbenches ${_target_name})
    endif()
endmacro()

# Helper macro to register an RFNoC block directory.
# Such a directory must always have a Makefiles.srcs containing all the
# required HDL files for synthesis, and optionally a Makefile file for running
# the testbench.
# The NOTESTBENCH argument can be used to skip the testbench target generation.
macro(RFNOC_REGISTER_BLOCK_DIR)
    cmake_parse_arguments(_rfnoc_block "NOTESTBENCH" "" "" ${ARGN})
    get_filename_component(_blk_name "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
    message(STATUS "Registering RFNoC block: ${_blk_name}")
    file(READ ${CMAKE_CURRENT_SOURCE_DIR}/Makefile.srcs _makefile_srcs)
    list(APPEND _block_src_files "Makefile.srcs")
    # Note: CMake uses an old regex engine that does not (among other things)
    # support word boundaries. When changing this regex, make sure that it
    # fully captures intended file names. For example, file.v is a subset of
    # file.vhd, so the regex should match file.vhd first.
    string(REGEX MATCHALL "[a-z0-9_]+\\.(vhdl?|s?v)" _src_files ${_makefile_srcs})
    foreach(_src_file ${_src_files})
        string(STRIP "${_src_file}" _src_file})
        list(APPEND _block_src_files "${_src_file}")
    endforeach()
    install(FILES ${_block_src_files}
        DESTINATION ${GATEWARE_PKG_DATA_DIR}/${_blk_name}
        COMPONENT gateware)
    if(NOT ${_rfnoc_block_NOTESTBENCH})
        RFNOC_ADD_TB_DIR()
    endif()
endmacro()

macro(RFNOC_REGISTER_IMAGE_CORE)
    cmake_parse_arguments(_rfnoc_image_core "" "SRC" "" ${ARGN})
    get_filename_component(_target_name ${_rfnoc_image_core_SRC} NAME_WE)
    if(NOT _target_name MATCHES "image_core")
        message(FATAL_ERROR
            "Invalid image core source file name: ${_rfnoc_image_core_SRC} (must end in `image_core`)")
    endif()
    if (_rfnoc_image_builder_exe)
        message(STATUS "Adding image core target: ${_target_name}")
        add_custom_target(${_target_name}
            COMMAND ${_rfnoc_image_builder_exe} -F ${UHD_FPGA_DIR} -y ${CMAKE_CURRENT_SOURCE_DIR}/${_rfnoc_image_core_SRC} -I ${CMAKE_SOURCE_DIR} -B ${CMAKE_BINARY_DIR}/build-${_target_name} -O ${CMAKE_BINARY_DIR} -E ${CMAKE_BINARY_DIR}/build-ip
        )
    endif()
endmacro()

########################################################################
# Create uninstall target
########################################################################
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
@ONLY)
add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)

########################################################################
# Install cmake search helper for this library
########################################################################
if(NOT CMAKE_MODULES_DIR)
    set(CMAKE_MODULES_DIR lib${LIB_SUFFIX}/cmake)
endif(NOT CMAKE_MODULES_DIR)

#TODO
#install(FILES cmake/Modules/RfnocExampleConfig.cmake
#    DESTINATION ${CMAKE_MODULES_DIR}/rfnoc
#)

########################################################################
# Subdirectories
########################################################################
if(UHD_FPGA_DIR)
    add_subdirectory(rfnoc)
    add_subdirectory(fpga/gain)
    add_subdirectory(icores)
endif()
if(UHD_FOUND)
    add_subdirectory(include/rfnoc/gain)
    add_subdirectory(lib)
    add_subdirectory(apps)
    add_subdirectory(examples)
endif()
if(ENABLE_PYTHON_API)
    add_subdirectory(python)
endif()

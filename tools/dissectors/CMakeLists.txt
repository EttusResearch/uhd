# Copyright 2010-2013 Ettus Research LLC
# Copyright 2019 Ettus Research, a National Instruments brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

cmake_minimum_required(VERSION 3.1)
cmake_policy(SET CMP0048 NEW)
project(EttusDissectors VERSION 4.0.0 DESCRIPTION "Dissector Plugins" LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
set(UHD_HOST_ROOT ${CMAKE_SOURCE_DIR}/../../host)

########################################################################
# useful macros
########################################################################
include(WSComponent)

set(PLUGIN_C_GENERATOR ${CMAKE_SOURCE_DIR}/make-plugin-reg.py)
macro(register_plugin_files _outputfile _registertype)

    add_custom_command(
        OUTPUT ${_outputfile}
        DEPENDS ${ARGN} ${PLUGIN_C_GENERATOR}
        COMMAND ${PYTHON_EXECUTABLE}
        ${PLUGIN_C_GENERATOR}
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${_registertype}
        ${ARGN}
        COMMENT "Generating ${_outputfile}"
    )

endmacro(register_plugin_files)

# Plugin name and version info (major minor micro extra)
macro(SET_MODULE_INFO _plugin _ver_major _ver_minor _ver_micro _ver_extra)
    if(WIN32)
        # Create the Windows .rc file for the plugin.
        set(MODULE_NAME ${_plugin})
        set(MODULE_VERSION_MAJOR ${_ver_major})
        set(MODULE_VERSION_MINOR ${_ver_minor})
        set(MODULE_VERSION_MICRO ${_ver_micro})
        set(MODULE_VERSION_EXTRA ${_ver_extra})
        set(MODULE_VERSION "${MODULE_VERSION_MAJOR}.${MODULE_VERSION_MINOR}.${MODULE_VERSION_MICRO}.${MODULE_VERSION_EXTRA}")
        set(RC_MODULE_VERSION "${MODULE_VERSION_MAJOR},${MODULE_VERSION_MINOR},${MODULE_VERSION_MICRO},${MODULE_VERSION_EXTRA}")

        set(MSVC_VARIANT "${CMAKE_GENERATOR}")

        # Create the plugin.rc file from the template
        set(_plugin_rc_in ${CMAKE_CURRENT_SOURCE_DIR}/plugin.rc.in)
        configure_file(${_plugin_rc_in} plugin.rc @ONLY)
        set(PLUGIN_RC_FILE ${CMAKE_CURRENT_BINARY_DIR}/plugin.rc)
    endif()

    set(PLUGIN_VERSION "${_ver_major}.${_ver_minor}.${_ver_micro}")
    add_definitions(-DPLUGIN_VERSION=\"${PLUGIN_VERSION}\")
    add_definitions(-DVERSION_MAJOR=${Wireshark_VERSION_MAJOR})
    add_definitions(-DVERSION_MINOR=${Wireshark_VERSION_MINOR})
endmacro()

macro(ADD_PLUGIN_LIBRARY _plugin _subfolder)
    add_library(${_plugin} MODULE
        ${PLUGIN_FILES}
        ${PLUGIN_RC_FILE}
    )

    set_target_properties(${_plugin} PROPERTIES
        PREFIX ""
        LINK_FLAGS "${WS_LINK_FLAGS}"
        FOLDER "Plugins"
    )

    set_target_properties(${_plugin} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${_subfolder}
        INSTALL_RPATH ""
    )
endmacro()

macro(INSTALL_PLUGIN _plugin _subfolder)
    install(TARGETS ${_plugin}
        LIBRARY DESTINATION ${_subfolder} NAMELINK_SKIP
    )
    #LIBRARY DESTINATION ${Wireshark_PLUGIN_INSTALL_DIR}/${_subfolder} NAMELINK_SKIP
    #RUNTIME DESTINATION ${Wireshark_PLUGIN_INSTALL_DIR}
    #ARCHIVE DESTINATION ${Wireshark_PLUGIN_INSTALL_DIR}
endmacro()


find_package(Wireshark CONFIG REQUIRED)
find_package(GLIB2)

if(NOT Wireshark_PLUGINS_ENABLED)
    message(FATAL_ERROR "Wireshark was compiled without support for plugins")
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${Wireshark_PLUGIN_INSTALL_DIR}
        CACHE
        PATH
        "Default installation path for plugins"
        FORCE
    )
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

set(UHD_INCLUDE_DIR "${UHD_HOST_ROOT}/include")
set(UHDLIB_INCLUDE_DIR "${UHD_HOST_ROOT}/lib/include")

include_directories(
    ${UHD_INCLUDE_DIR}
    ${UHDLIB_INCLUDE_DIR}
    ${GLIB2_INCLUDE_DIRS}
    ${Wireshark_INCLUDE_DIR}
)

WS_REGISTER_COMPONENT("RFNoC" ENABLE_RFNOC ON "" OFF ON)
WS_REGISTER_COMPONENT("Octoclock" ENABLE_OCTOCLOCK ON "" OFF OFF)
WS_REGISTER_COMPONENT("ZPU" ENABLE_ZPU ON "" OFF OFF)

if(NOT DEFINED RFNOC_CHDR_WIDTH)
    set(RFNOC_CHDR_WIDTH 64)
endif()
add_definitions(-DRFNOC_CHDR_WIDTH=${RFNOC_CHDR_WIDTH})

if(ENABLE_RFNOC)
    add_subdirectory(epan/rfnoc)
endif(ENABLE_RFNOC)

if(ENABLE_OCTOCLOCK)
    add_subdirectory(epan/octoclock)
endif(ENABLE_OCTOCLOCK)

if(ENABLE_ZPU)
    add_subdirectory(epan/zpu)
endif(ENABLE_ZPU)

WS_PRINT_COMPONENT_SUMMARY()

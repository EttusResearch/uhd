# Copyright 2019 Ettus Research, a National Instruments brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# useful macros
########################################################################
include(WSComponent)

set(PLUGIN_C_GENERATOR ${CMAKE_SOURCE_DIR}/make-dissector-reg-ws2.py)
macro(REGISTER_PLUGIN_FILES _plugin _outputfile _registertype)

    add_custom_command(
            OUTPUT ${_outputfile}
	    DEPENDS ${ARGN} ${PLUGIN_C_GENERATOR}
	    COMMAND ${PYTHON_EXECUTABLE}
	        ${PLUGIN_C_GENERATOR}
		${CMAKE_CURRENT_SOURCE_DIR}
		${_registertype}
                ${_plugin}
		${ARGN}
            COMMENT "Generating ${_outputfile}"
    )

endmacro(REGISTER_PLUGIN_FILES)

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
	add_definitions(-DVERSION_MAJOR=${WIRESHARK_VERSION_MAJOR})
	add_definitions(-DVERSION_MINOR=${WIRESHARK_VERSION_MINOR})
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
        LIBRARY DESTINATION ${WIRESHARK_PLUGIN_DIR}/${_subfolder} NAMELINK_SKIP
    )
endmacro()

if(NOT WIRESHARK_PLUGIN_DIR)
    message(FATAL_ERROR "Wireshark was compiled without support for plugins")
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX ${WIRESHARK_PLUGIN_DIR}
        CACHE
        PATH
        "Default installation path for plugins"
        FORCE
    )
endif(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)

WS_REGISTER_COMPONENT("CHDR" ENABLE_CHDR ON "" OFF ON)
WS_REGISTER_COMPONENT("Octoclock" ENABLE_OCTOCLOCK ON "" OFF OFF)
WS_REGISTER_COMPONENT("ZPU" ENABLE_ZPU ON "" OFF OFF)

if(ENABLE_CHDR)
    include(epan_chdr.cmake)
endif(ENABLE_CHDR)

if(ENABLE_OCTOCLOCK)
    include(epan_octoclock.cmake)
endif(ENABLE_OCTOCLOCK)

if(ENABLE_ZPU)
    include(epan_zpu.cmake)
endif(ENABLE_ZPU)

WS_PRINT_COMPONENT_SUMMARY()

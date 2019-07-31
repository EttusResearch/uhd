#
# Copyright 2019 Ettus Research, a National Instruments brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
set(_ws_enabled_components "" CACHE INTERNAL "" FORCE)
set(_ws_disabled_components "" CACHE INTERNAL "" FORCE)

########################################################################
# Register a component into the system
#  - name the component string name ("FOO")
#  - var the global enable variable (ENABLE_FOO)
#  - enb the default enable setting (ON)
#  - deps a list of dependencies (DEPENDENCY_FOUND)
#  - dis the default disable setting (OFF)
#  - req fail if dependencies not met (unless specifically disabled)
#
# In parentheses are examples. If you specify those, we register a component
# "FOO" which is enabled by calling CMake with -DENABLE_FOO=ON. It defaults to
# ON, unless DEPENDENCY_FOUND is false, in which case it becomes false.
########################################################################
macro(WS_REGISTER_COMPONENT name var enb deps dis req)
    message(STATUS "")
    message(STATUS "Configuring ${name} support...")
    foreach(dep ${deps})
        message(STATUS "  Dependency ${dep} = ${${dep}}")
    endforeach(dep)

    # If user specified option, store here. Note: If the user doesn't specify
    # this option on the cmake command line, both user_enabled and
    # user_disabled will be false!
    if("${${var}}" STREQUAL "OFF")
        set(user_disabled TRUE)
    else()
        set(user_disabled FALSE)
    endif("${${var}}" STREQUAL "OFF")
    if("${${var}}" STREQUAL "ON")
        set(user_enabled TRUE)
    else()
        set(user_enabled FALSE)
    endif("${${var}}" STREQUAL "ON")

    # Override default if user set
    if(user_enabled OR user_disabled)
        set(option "${${var}}")
    else(user_enabled OR user_disabled)
        set(option ${req})
    endif()

    # setup the dependent option for this component
    include(CMakeDependentOption)
    CMAKE_DEPENDENT_OPTION(${var} "enable ${name} support" ${option} "${deps}" ${dis})

    # There are two failure cases:
    # 1) The user requested this component explicitly (-DENABLE_FOO=ON) but the
    #    requirements are not met.
    # 2) The user did not explicitly turn off this component (-DENABLE_FOO=OFF)
    #    but it is flagged as required by ${req}
    if(NOT ${var} AND user_enabled) # Case 1)
        message(FATAL_ERROR "Dependencies for required component ${name} not met.")
    endif(NOT ${var} AND user_enabled)
    if(NOT ${var} AND ${req} AND NOT user_disabled) # Case 2)
        message(FATAL_ERROR "Dependencies for required component ${name} not met.")
    endif(NOT ${var} AND ${req} AND NOT user_disabled)

    #append the component into one of the lists
    if(${var})
        message(STATUS "  Enabling ${name} support.")
        list(APPEND _ws_enabled_components ${name})
    else(${var})
        message(STATUS "  Disabling ${name} support.")
        list(APPEND _ws_disabled_components ${name})
    endif(${var})
    message(STATUS "  Override with -D${var}=ON/OFF")

    #make components lists into global variables
    set(_ws_enabled_components ${_ws_enabled_components} CACHE INTERNAL "" FORCE)
    set(_ws_disabled_components ${_ws_disabled_components} CACHE INTERNAL "" FORCE)
endmacro(WS_REGISTER_COMPONENT)

########################################################################
# Print the registered component summary
########################################################################
function(WS_PRINT_COMPONENT_SUMMARY)
    message(STATUS "")
    message(STATUS "######################################################")
    message(STATUS "# Ettus dissectors enabled                            ")
    message(STATUS "######################################################")
    foreach(comp ${_ws_enabled_components})
        message(STATUS "  * ${comp}")
    endforeach(comp)

    message(STATUS "")
    message(STATUS "######################################################")
    message(STATUS "# Ettus dissectors disabled                           ")
    message(STATUS "######################################################")
    foreach(comp ${_ws_disabled_components})
        message(STATUS "  * ${comp}")
    endforeach(comp)

    message(STATUS "")
endfunction(WS_PRINT_COMPONENT_SUMMARY)

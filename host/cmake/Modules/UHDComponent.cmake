#
# Copyright 2010-2011,2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
set(_uhd_enabled_components "" CACHE INTERNAL "" FORCE)
set(_uhd_disabled_components "" CACHE INTERNAL "" FORCE)

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
macro(LIBUHD_REGISTER_COMPONENT name var enb deps dis req)
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

    #setup the dependent option for this component
    include(CMakeDependentOption)
    CMAKE_DEPENDENT_OPTION(${var} "enable ${name} support" ${enb} "${deps}" ${dis})

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
        list(APPEND _uhd_enabled_components ${name})
    else(${var})
        message(STATUS "  Disabling ${name} support.")
        list(APPEND _uhd_disabled_components ${name})
    endif(${var})
    message(STATUS "  Override with -D${var}=ON/OFF")

    #make components lists into global variables
    set(_uhd_enabled_components ${_uhd_enabled_components} CACHE INTERNAL "" FORCE)
    set(_uhd_disabled_components ${_uhd_disabled_components} CACHE INTERNAL "" FORCE)
endmacro(LIBUHD_REGISTER_COMPONENT)

########################################################################
# Install only if appropriate for package and if component is enabled
########################################################################
function(UHD_INSTALL)
    include(CMakeParseArguments)
    cmake_parse_arguments(UHD_INSTALL "" "DESTINATION;COMPONENT" "TARGETS;FILES;PROGRAMS" ${ARGN})

    if(UHD_INSTALL_FILES)
        set(TO_INSTALL "${UHD_INSTALL_FILES}")
    elseif(UHD_INSTALL_PROGRAMS)
        set(TO_INSTALL "${UHD_INSTALL_PROGRAMS}")
    elseif(UHD_INSTALL_TARGETS)
        set(TO_INSTALL "${UHD_INSTALL_TARGETS}")
    endif(UHD_INSTALL_FILES)

    if(UHD_INSTALL_COMPONENT STREQUAL "headers")
        if(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
            install(${ARGN})
        endif(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
    elseif(UHD_INSTALL_COMPONENT STREQUAL "devel")
        if(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
            install(${ARGN})
        endif(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
    elseif(UHD_INSTALL_COMPONENT STREQUAL "examples")
        if(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
            install(${ARGN})
        endif(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
    elseif(UHD_INSTALL_COMPONENT STREQUAL "tests")
        if(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
            install(${ARGN})
        endif(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
    elseif(UHD_INSTALL_COMPONENT STREQUAL "utilities")
        if(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
            install(${ARGN})
        endif(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
    elseif(UHD_INSTALL_COMPONENT STREQUAL "manual")
        if(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
            install(${ARGN})
        endif(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
    elseif(UHD_INSTALL_COMPONENT STREQUAL "doxygen")
        if(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
            install(${ARGN})
        endif(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
    elseif(UHD_INSTALL_COMPONENT STREQUAL "manpages")
        if(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
            install(${ARGN})
        endif(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
    elseif(UHD_INSTALL_COMPONENT STREQUAL "images")
        if(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG AND NOT UHDHOST_PKG)
            install(${ARGN})
        endif(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG AND NOT UHDHOST_PKG)
    elseif(UHD_INSTALL_COMPONENT STREQUAL "readme")
        if(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG AND NOT UHDHOST_PKG)
            install(${ARGN})
        endif(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG AND NOT UHDHOST_PKG)
    endif(UHD_INSTALL_COMPONENT STREQUAL "headers")
endfunction(UHD_INSTALL)

########################################################################
# Print the registered component summary
########################################################################
function(UHD_PRINT_COMPONENT_SUMMARY)
    message(STATUS "")
    message(STATUS "######################################################")
    message(STATUS "# UHD enabled components                              ")
    message(STATUS "######################################################")
    foreach(comp ${_uhd_enabled_components})
        message(STATUS "  * ${comp}")
    endforeach(comp)

    message(STATUS "")
    message(STATUS "######################################################")
    message(STATUS "# UHD disabled components                             ")
    message(STATUS "######################################################")
    foreach(comp ${_uhd_disabled_components})
        message(STATUS "  * ${comp}")
    endforeach(comp)

    message(STATUS "")
endfunction(UHD_PRINT_COMPONENT_SUMMARY)

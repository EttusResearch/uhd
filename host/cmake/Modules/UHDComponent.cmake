#
# Copyright 2010-2011,2013,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
SET(_uhd_enabled_components "" CACHE INTERNAL "" FORCE)
SET(_uhd_disabled_components "" CACHE INTERNAL "" FORCE)

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
MACRO(LIBUHD_REGISTER_COMPONENT name var enb deps dis req)
    MESSAGE(STATUS "")
    MESSAGE(STATUS "Configuring ${name} support...")
    FOREACH(dep ${deps})
        MESSAGE(STATUS "  Dependency ${dep} = ${${dep}}")
    ENDFOREACH(dep)

    # If user specified option, store here. Note: If the user doesn't specify
    # this option on the cmake command line, both user_enabled and
    # user_disabled will be false!
    IF("${${var}}" STREQUAL "OFF")
        SET(user_disabled TRUE)
    ELSE()
        SET(user_disabled FALSE)
    ENDIF("${${var}}" STREQUAL "OFF")
    IF("${${var}}" STREQUAL "ON")
        SET(user_enabled TRUE)
    ELSE()
        SET(user_enabled FALSE)
    ENDIF("${${var}}" STREQUAL "ON")

    #setup the dependent option for this component
    INCLUDE(CMakeDependentOption)
    CMAKE_DEPENDENT_OPTION(${var} "enable ${name} support" ${enb} "${deps}" ${dis})

    # There are two failure cases:
    # 1) The user requested this component explicitly (-DENABLE_FOO=ON) but the
    #    requirements are not met.
    # 2) The user did not explicitly turn off this component (-DENABLE_FOO=OFF)
    #    but it is flagged as required by ${req}
    IF(NOT ${var} AND user_enabled) # Case 1)
        MESSAGE(FATAL_ERROR "Dependencies for required component ${name} not met.")
    ENDIF(NOT ${var} AND user_enabled)
    IF(NOT ${var} AND ${req} AND NOT user_disabled) # Case 2)
        MESSAGE(FATAL_ERROR "Dependencies for required component ${name} not met.")
    ENDIF(NOT ${var} AND ${req} AND NOT user_disabled)

    #append the component into one of the lists
    IF(${var})
        MESSAGE(STATUS "  Enabling ${name} support.")
        LIST(APPEND _uhd_enabled_components ${name})
    ELSE(${var})
        MESSAGE(STATUS "  Disabling ${name} support.")
        LIST(APPEND _uhd_disabled_components ${name})
    ENDIF(${var})
    MESSAGE(STATUS "  Override with -D${var}=ON/OFF")

    #make components lists into global variables
    SET(_uhd_enabled_components ${_uhd_enabled_components} CACHE INTERNAL "" FORCE)
    SET(_uhd_disabled_components ${_uhd_disabled_components} CACHE INTERNAL "" FORCE)
ENDMACRO(LIBUHD_REGISTER_COMPONENT)

########################################################################
# Install only if appropriate for package and if component is enabled
########################################################################
FUNCTION(UHD_INSTALL)
    include(CMakeParseArgumentsCopy)
    CMAKE_PARSE_ARGUMENTS(UHD_INSTALL "" "DESTINATION;COMPONENT" "TARGETS;FILES;PROGRAMS" ${ARGN})

    IF(UHD_INSTALL_FILES)
        SET(TO_INSTALL "${UHD_INSTALL_FILES}")
    ELSEIF(UHD_INSTALL_PROGRAMS)
        SET(TO_INSTALL "${UHD_INSTALL_PROGRAMS}")
    ELSEIF(UHD_INSTALL_TARGETS)
        SET(TO_INSTALL "${UHD_INSTALL_TARGETS}")
    ENDIF(UHD_INSTALL_FILES)

    IF(UHD_INSTALL_COMPONENT STREQUAL "headers")
        IF(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
            INSTALL(${ARGN})
        ENDIF(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
    ELSEIF(UHD_INSTALL_COMPONENT STREQUAL "devel")
        IF(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
            INSTALL(${ARGN})
        ENDIF(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
    ELSEIF(UHD_INSTALL_COMPONENT STREQUAL "examples")
        IF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
            INSTALL(${ARGN})
        ENDIF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
    ELSEIF(UHD_INSTALL_COMPONENT STREQUAL "tests")
        IF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
            INSTALL(${ARGN})
        ENDIF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
    ELSEIF(UHD_INSTALL_COMPONENT STREQUAL "utilities")
        IF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
            INSTALL(${ARGN})
        ENDIF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
    ELSEIF(UHD_INSTALL_COMPONENT STREQUAL "manual")
        IF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
            INSTALL(${ARGN})
        ENDIF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
    ELSEIF(UHD_INSTALL_COMPONENT STREQUAL "doxygen")
        IF(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
            INSTALL(${ARGN})
        ENDIF(NOT LIBUHD_PKG AND NOT UHDHOST_PKG)
    ELSEIF(UHD_INSTALL_COMPONENT STREQUAL "manpages")
        IF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
            INSTALL(${ARGN})
        ENDIF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG)
    ELSEIF(UHD_INSTALL_COMPONENT STREQUAL "images")
        IF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG AND NOT UHDHOST_PKG)
            INSTALL(${ARGN})
        ENDIF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG AND NOT UHDHOST_PKG)
    ELSEIF(UHD_INSTALL_COMPONENT STREQUAL "readme")
        IF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG AND NOT UHDHOST_PKG)
            INSTALL(${ARGN})
        ENDIF(NOT LIBUHD_PKG AND NOT LIBUHDDEV_PKG AND NOT UHDHOST_PKG)
    ENDIF(UHD_INSTALL_COMPONENT STREQUAL "headers")
ENDFUNCTION(UHD_INSTALL)

########################################################################
# Print the registered component summary
########################################################################
FUNCTION(UHD_PRINT_COMPONENT_SUMMARY)
    MESSAGE(STATUS "")
    MESSAGE(STATUS "######################################################")
    MESSAGE(STATUS "# UHD enabled components                              ")
    MESSAGE(STATUS "######################################################")
    FOREACH(comp ${_uhd_enabled_components})
        MESSAGE(STATUS "  * ${comp}")
    ENDFOREACH(comp)

    MESSAGE(STATUS "")
    MESSAGE(STATUS "######################################################")
    MESSAGE(STATUS "# UHD disabled components                             ")
    MESSAGE(STATUS "######################################################")
    FOREACH(comp ${_uhd_disabled_components})
        MESSAGE(STATUS "  * ${comp}")
    ENDFOREACH(comp)

    MESSAGE(STATUS "")
ENDFUNCTION(UHD_PRINT_COMPONENT_SUMMARY)

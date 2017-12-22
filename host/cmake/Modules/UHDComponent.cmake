#
# Copyright 2010-2011,2013,2015 Ettus Research LLC
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
SET(_uhd_enabled_components "" CACHE INTERNAL "" FORCE)
SET(_uhd_disabled_components "" CACHE INTERNAL "" FORCE)

########################################################################
# Register a component into the system
#  - name the component string name
#  - var the global enable variable
#  - enb the default enable setting
#  - deps a list of dependencies
#  - dis the default disable setting
#  - req fail if dependencies not met (unless specifically disabled)
########################################################################
MACRO(LIBUHD_REGISTER_COMPONENT name var enb deps dis req)
    MESSAGE(STATUS "")
    MESSAGE(STATUS "Configuring ${name} support...")
    FOREACH(dep ${deps})
        MESSAGE(STATUS "  Dependency ${dep} = ${${dep}}")
    ENDFOREACH(dep)

    #if user specified option, store here
    IF("${${var}}" STREQUAL "OFF")
        SET(user_disabled TRUE)
    ELSE()
        SET(user_disabled FALSE)
    ENDIF("${${var}}" STREQUAL "OFF")

    #setup the dependent option for this component
    INCLUDE(CMakeDependentOption)
    CMAKE_DEPENDENT_OPTION(${var} "enable ${name} support" ${enb} "${deps}" ${dis})

    #if a required option's dependencies aren't met, fail unless user specifies otherwise
    IF(NOT ${var} AND ${req} AND NOT user_disabled)
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

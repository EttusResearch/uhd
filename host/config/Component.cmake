#
# Copyright 2010 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
########################################################################
FUNCTION(LIBUHD_REGISTER_COMPONENT name var enb deps dis)
    INCLUDE(CMakeDependentOption)
    MESSAGE(STATUS "")
    MESSAGE(STATUS "Configuring ${name} support...")
    IF(DEFINED ${var})
        MESSAGE(STATUS "${name} support configured ${var}=${${var}}")
    ELSE(DEFINED ${var}) #not defined: automatic enabling of component
        MESSAGE(STATUS "${name} support configured automatically")
    ENDIF(DEFINED ${var})

    #setup the dependent option for this component
    CMAKE_DEPENDENT_OPTION(${var} "enable ${name} support" ${enb} "${deps}" ${dis})

    #remove previous occurrence of component in either list
    IF(DEFINED _uhd_enabled_components)
        LIST(REMOVE_ITEM _uhd_enabled_components ${name})
    ENDIF(DEFINED _uhd_enabled_components)
    IF(DEFINED _uhd_disabled_components)
        LIST(REMOVE_ITEM _uhd_disabled_components ${name})
    ENDIF(DEFINED _uhd_disabled_components)

    #append the component into one of the lists
    IF(${var})
        MESSAGE(STATUS "  Enabling ${name} support.")
        LIST(APPEND _uhd_enabled_components ${name})
    ELSE(${var})
        MESSAGE(STATUS "  Disabling ${name} support.")
        LIST(APPEND _uhd_disabled_components ${name})
    ENDIF(${var})

    #make components lists into global variables
    SET(_uhd_enabled_components ${_uhd_enabled_components} CACHE INTERNAL "" FORCE)
    SET(_uhd_disabled_components ${_uhd_disabled_components} CACHE INTERNAL "" FORCE)
ENDFUNCTION(LIBUHD_REGISTER_COMPONENT)

########################################################################
# Print the registered component summary
########################################################################
FUNCTION(UHD_PRINT_COMPONENT_SUMMARY)
    MESSAGE(STATUS "")
    MESSAGE(STATUS "######################################################")
    MESSAGE(STATUS "# LibUHD enabled components                           ")
    MESSAGE(STATUS "######################################################")
    FOREACH(comp ${_uhd_enabled_components})
        MESSAGE(STATUS "  * ${comp}")
    ENDFOREACH(comp)

    MESSAGE(STATUS "")
    MESSAGE(STATUS "######################################################")
    MESSAGE(STATUS "# LibUHD disabled components                          ")
    MESSAGE(STATUS "######################################################")
    FOREACH(comp ${_uhd_disabled_components})
        MESSAGE(STATUS "  * ${comp}")
    ENDFOREACH(comp)

    MESSAGE(STATUS "")
ENDFUNCTION(UHD_PRINT_COMPONENT_SUMMARY)

#
# Copyright 2010-2011 Ettus Research LLC
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
MACRO(LIBUHD_REGISTER_COMPONENT name var enb deps dis)
    MESSAGE(STATUS "")
    MESSAGE(STATUS "Configuring ${name} support...")
    FOREACH(dep ${deps})
        MESSAGE(STATUS "  Dependency ${dep} = ${${dep}}")
    ENDFOREACH(dep)

    #setup the dependent option for this component
    INCLUDE(CMakeDependentOption)
    CMAKE_DEPENDENT_OPTION(${var} "enable ${name} support" ${enb} "${deps}" ${dis})

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

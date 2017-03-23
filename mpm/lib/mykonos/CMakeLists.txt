#
# Copyright 2017 Ettus Research (National Instruments)
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
# This file included, use CMake directory variables
########################################################################

MACRO(MYKONOS_APPEND_SOURCES)
    SET(mykonos_sources ${mykonos_sources})
    LIST(APPEND mykonos_sources ${ARGV})
    SET(mykonos_sources ${mykonos_sources} PARENT_SCOPE)
ENDMACRO(MYKONOS_APPEND_SOURCES)

SET(mykonos_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/ad937x_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/ad937x_device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adi_ctrl.cpp
)

ADD_SUBDIRECTORY(adi)
ADD_SUBDIRECTORY(config)

USRP_PERIPHS_ADD_OBJECT(mykonos ${mykonos_sources})

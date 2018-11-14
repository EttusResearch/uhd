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
include(FindPythonInterp)

macro(GEN_RMI target)
    get_filename_component(name ${target} NAME_WE)
    #command to create a rmi from elf
    add_custom_command(
        OUTPUT ${name}.rmi DEPENDS ${name}.bin
        COMMAND ${PYTHON_EXECUTABLE}
        ${CMAKE_SOURCE_DIR}/bin/bin_to_ram_macro_init.py ${name}.bin ${name}.rmi
    )
    #add a top level target for output files
    add_custom_target(
        ${name}_rmi ALL DEPENDS ${name}.rmi
    )
endmacro(GEN_RMI)

########################################################################
add_executable(bootloader.elf ${CMAKE_SOURCE_DIR}/apps/txrx_uhd.c)
add_definitions(-DUSRP2P)
add_definitions(-DBOOTLOADER)
target_link_libraries(bootloader.elf libusrp2pfw)
set(GEN_OUTPUTS_BIN_SIZE 0x3fff)
GEN_OUTPUTS(bootloader.elf)
GEN_RMI(bootloader.bin)

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
# setup project and compiler
########################################################################
cmake_minimum_required(VERSION 2.6)
#force the compiler because the check wont use the special flag below
include(CMakeForceCompiler)
set(CMAKE_SYSTEM_NAME Generic)
CMAKE_FORCE_C_COMPILER(zpu-elf-gcc GNU)
project(USRP_NXXX_FW C)

########################################################################
# lwIP header include dirs
########################################################################
set(LWIPDIR ${CMAKE_SOURCE_DIR}/lwip/lwip-1.3.1)

include_directories(
    ${CMAKE_SOURCE_DIR}/lwip
    ${CMAKE_SOURCE_DIR}/lwip_port
    ${LWIPDIR}/src/include
    ${LWIPDIR}/src/include/ipv4
)

########################################################################
# misc flags for the gcc compiler
########################################################################
set(CMAKE_C_FLAGS -phi) #always needed compile time and link time
add_definitions(-Os)
add_definitions(--std=gnu99)
add_definitions(-Wall)
add_definitions(-Werror-implicit-function-declaration)
add_definitions(-ffunction-sections)

macro(ADD_LINKER_FLAGS flags)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${flags}")
endmacro(ADD_LINKER_FLAGS)

ADD_LINKER_FLAGS("-Wl,--gc-sections")
ADD_LINKER_FLAGS("-Wl,--relax")

########################################################################
# define for the hal io (FIXME move?)
########################################################################
#add_definitions(-DHAL_IO_USES_DBOARD_PINS)
add_definitions(-DHAL_IO_USES_UART)

########################################################################
# common cflags and ldflags
########################################################################
include_directories(
    ${CMAKE_SOURCE_DIR}/../../host/lib/usrp
    ${CMAKE_SOURCE_DIR}/lib
)

########################################################################
# setup programs for output files
########################################################################
find_program(LINKER zpu-elf-ld)
find_program(OBJCOPY zpu-elf-objcopy)
find_program(OBJDUMP zpu-elf-objdump)
find_program(HEXDUMP hexdump)

########################################################################
# helper functions to build output formats
########################################################################
set(GEN_OUTPUTS_BIN_SIZE "bin_size_not_set") #set before calling
macro(GEN_OUTPUTS target)
    get_filename_component(name ${target} NAME_WE)
    #command to create a map from elf
    add_custom_command(
        OUTPUT ${name}.map DEPENDS ${target}
        COMMAND ${LINKER} -Map ${name}.map ${target}
    )
    #command to create a bin from elf
    add_custom_command(
        OUTPUT ${name}.bin DEPENDS ${target}
        COMMAND ${OBJCOPY} -O binary ${target} ${name}.bin
        --pad-to ${GEN_OUTPUTS_BIN_SIZE}
    )
    #command to create a ihx from elf
    add_custom_command(
        OUTPUT ${name}.ihx DEPENDS ${target}
        COMMAND ${OBJCOPY} -O ihex ${target} ${name}.ihx
        --pad-to ${GEN_OUTPUTS_BIN_SIZE}
    )
    #command to create a dump from elf
    add_custom_command(
        OUTPUT ${name}.dump DEPENDS ${target}
        COMMAND ${OBJDUMP} -DSC ${target} > ${name}.dump
    )
    #command to create a rom from bin
    add_custom_command(
        OUTPUT ${name}.rom DEPENDS ${name}.bin
        COMMAND ${HEXDUMP} -v -e'1/1 \"%.2X\\n\"' ${name}.bin > ${name}.rom
    )
    #add a top level target for output files
    add_custom_target(
        ${name}_outputs ALL DEPENDS ${name}.map ${name}.bin ${name}.ihx ${name}.dump ${name}.rom
    )
endmacro(GEN_OUTPUTS)

########################################################################
# Add the subdirectories
########################################################################
add_subdirectory(usrp2)
add_subdirectory(usrp2p)

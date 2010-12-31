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
# setup project and compiler
########################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
SET(CMAKE_C_COMPILER zpu-elf-gcc)
SET(CMAKE_C_COMPILER_WORKS TRUE)
SET(CMAKE_C_COMPILER_FORCED TRUE)
PROJECT(USRP_NXXX_FW C)

########################################################################
# lwIP header include dirs
########################################################################
SET(LWIPDIR ${CMAKE_SOURCE_DIR}/lwip/lwip-1.3.1)

INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/lwip
    ${CMAKE_SOURCE_DIR}/lwip_port
    ${LWIPDIR}/src/include
    ${LWIPDIR}/src/include/ipv4
)

########################################################################
# misc flags for the gcc compiler
########################################################################
SET(CMAKE_C_FLAGS -phi) #always needed compile time and link time
ADD_DEFINITIONS(-Os --std=gnu99 -Wall -Werror-implicit-function-declaration -ffunction-sections)

MACRO(ADD_LINKER_FLAGS flags)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${flags}")
ENDMACRO(ADD_LINKER_FLAGS)

ADD_LINKER_FLAGS("-Wl,--relax -Wl,--gc-sections")

########################################################################
# define for the hal io (FIXME move?)
########################################################################
#ADD_DEFINITIONS(-DHAL_IO_USES_DBOARD_PINS)
ADD_DEFINITIONS(-DHAL_IO_USES_UART)

########################################################################
# common cflags and ldflags
########################################################################
INCLUDE_DIRECTORIES(
    ${CMAKE_SOURCE_DIR}/../../host/lib/usrp
    ${CMAKE_SOURCE_DIR}/lib
)

########################################################################
# setup programs for output files
########################################################################
FIND_PROGRAM(LINKER zpu-elf-ld)
FIND_PROGRAM(OBJCOPY zpu-elf-objcopy)
FIND_PROGRAM(OBJDUMP zpu-elf-objdump)
FIND_PROGRAM(HEXDUMP hexdump)

########################################################################
# helper functions to build output formats
########################################################################
MACRO(GEN_OUTPUTS target)
    GET_FILENAME_COMPONENT(name ${target} NAME_WE)
    #command to create a map from elf
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.map DEPENDS ${target}
        COMMAND ${LINKER}
        ARGS -Map ${name}.map ${target}
    )
    #command to create a bin from elf
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.bin DEPENDS ${target}
        COMMAND ${OBJCOPY}
        ARGS -O binary ${target} ${name}.bin
    )
    #command to create a ihx from elf
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.ihx DEPENDS ${target}
        COMMAND ${OBJCOPY}
        ARGS -O ihex ${target} ${name}.ihx
    )
    #command to create a dump from elf
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.dump DEPENDS ${target}
        COMMAND ${OBJDUMP}
        ARGS -DSC ${target} > ${name}.dump
    )
    #command to create a rom from bin
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.rom DEPENDS ${name}.bin
        COMMAND ${HEXDUMP}
        ARGS -v -e'1/1 \"%.2X\\n\"' ${name}.bin > ${name}.rom
    )
    #add a top level target for output files
    ADD_CUSTOM_TARGET(
        ${name}_outputs ALL DEPENDS ${name}.map ${name}.bin ${name}.ihx ${name}.dump ${name}.rom
    )
ENDMACRO(GEN_OUTPUTS)

########################################################################
# Add the subdirectories
########################################################################
ADD_SUBDIRECTORY(usrp2)
ADD_SUBDIRECTORY(usrp2p)

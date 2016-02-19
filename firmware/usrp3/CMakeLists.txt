#
# Copyright 2010-2014 Ettus Research LLC
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
#force the compiler because the check wont use the special flag below
INCLUDE(CMakeForceCompiler)
SET(CMAKE_SYSTEM_NAME Generic)
CMAKE_FORCE_C_COMPILER(zpu-elf-gcc GNU)
PROJECT(USRP3_FW C)

SET(UHD_VERSION_HASH 0 CACHE INTEGER "UHD Version Hash")
EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/utils/git-hash.sh OUTPUT_VARIABLE UHD_VERSION_HASH)
ADD_DEFINITIONS(-DUHD_VERSION_HASH=0x${UHD_VERSION_HASH})

INCLUDE_DIRECTORIES(include)

find_package(PythonInterp)

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
ADD_DEFINITIONS(-Os)
ADD_DEFINITIONS(--std=gnu99)
ADD_DEFINITIONS(-Wall)
ADD_DEFINITIONS(-Werror-implicit-function-declaration)
ADD_DEFINITIONS(-ffunction-sections)
ADD_DEFINITIONS(-DPRINTF_LONG_SUPPORT)

MACRO(ADD_LINKER_FLAGS flags)
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${flags}")
ENDMACRO(ADD_LINKER_FLAGS)

ADD_LINKER_FLAGS("-Wl,--gc-sections")
ADD_LINKER_FLAGS("-Wl,--relax")

########################################################################
# setup programs for output files
########################################################################
FIND_PROGRAM(LINKER zpu-elf-ld)
FIND_PROGRAM(OBJCOPY zpu-elf-objcopy)
FIND_PROGRAM(OBJDUMP zpu-elf-objdump)
FIND_PROGRAM(HEXDUMP hexdump)

########################################################################
# Firmware tracing support
########################################################################
# Look at include/trace.h to see what the different trace levels map to.
SET(TRACE_LEVEL "0" CACHE STRING "Firmware Trace Level") #0 by default
OPTION(TRACE_LEVEL "Firmware Trace Level" "") 
IF(TRACE_LEVEL)
    #If TRACE_LEVEL == 0, don't define UHD_FW_TRACE_LEVEL so that the C
    #code can easily detect if tracing is requested
    IF(${TRACE_LEVEL} GREATER 0)
        ADD_DEFINITIONS(-DUHD_FW_TRACE_LEVEL=${TRACE_LEVEL})
    ENDIF(${TRACE_LEVEL} GREATER 0)
ENDIF(TRACE_LEVEL)

########################################################################
# helper functions to build output formats
########################################################################
SET(GEN_OUTPUTS_BIN_SIZE "bin_size_not_set") #set before calling
MACRO(GEN_OUTPUTS target pretty_name)
    GET_FILENAME_COMPONENT(name ${target} NAME_WE)
    #command to create a map from elf
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.map DEPENDS ${target}
        COMMAND ${LINKER} -Map ${name}.map ${target}
    )
    #command to create a bin from elf
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.bin DEPENDS ${target}
        COMMAND ${OBJCOPY} -O binary ${target} ${name}.bin
        --pad-to ${GEN_OUTPUTS_BIN_SIZE}
    )
    #command to create a ihx from elf
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.ihx DEPENDS ${target}
        COMMAND ${OBJCOPY} -O ihex ${target} ${name}.ihx
        --pad-to ${GEN_OUTPUTS_BIN_SIZE}
    )
    #command to create a dump from elf
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.dump DEPENDS ${target}
        COMMAND ${OBJDUMP} -DSC ${target} > ${name}.dump
    )
    #command to create a rom from bin
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.rom DEPENDS ${name}.bin
        COMMAND ${HEXDUMP} -v -e'1/1 \"%.2X\\n\"' ${name}.bin > ${name}.rom
    )
    #command to create a coe from bin
    ADD_CUSTOM_COMMAND(
        OUTPUT ${name}.coe DEPENDS ${name}.bin
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/utils/bin_to_coe.py ${name}.bin ${name}.coe
    )
    #add a top level target for output files
    ADD_CUSTOM_TARGET(
        ${pretty_name} ALL DEPENDS ${name}.map ${name}.bin ${name}.ihx ${name}.dump ${name}.rom ${name}.coe
    )
ENDMACRO(GEN_OUTPUTS)

########################################################################
# Add the subdirectories
########################################################################
ADD_SUBDIRECTORY(lib)
ADD_SUBDIRECTORY(x300)
ADD_SUBDIRECTORY(n230)

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
cmake_minimum_required(VERSION 2.6)
#force the compiler because the check wont use the special flag below
include(CMakeForceCompiler)
set(CMAKE_SYSTEM_NAME Generic)
CMAKE_FORCE_C_COMPILER(zpu-elf-gcc GNU)
project(USRP3_FW C)

set(UHD_VERSION_HASH 0 CACHE INTEGER "UHD Version Hash")
execute_process(COMMAND ${CMAKE_SOURCE_DIR}/utils/git-hash.sh OUTPUT_VARIABLE UHD_VERSION_HASH)
add_definitions(-DUHD_VERSION_HASH=0x${UHD_VERSION_HASH})

include_directories(include)

find_package(PythonInterp)

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
add_definitions(-DPRINTF_LONG_SUPPORT)

macro(ADD_LINKER_FLAGS flags)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${flags}")
endmacro(ADD_LINKER_FLAGS)

ADD_LINKER_FLAGS("-Wl,--gc-sections")
ADD_LINKER_FLAGS("-Wl,--relax")

########################################################################
# setup programs for output files
########################################################################
find_program(LINKER zpu-elf-ld)
find_program(OBJCOPY zpu-elf-objcopy)
find_program(OBJDUMP zpu-elf-objdump)
find_program(HEXDUMP hexdump)

########################################################################
# Firmware tracing support
########################################################################
# Look at include/trace.h to see what the different trace levels map to.
set(TRACE_LEVEL "0" CACHE STRING "Firmware Trace Level") #0 by default
option(TRACE_LEVEL "Firmware Trace Level" "") 
if(TRACE_LEVEL)
    #If TRACE_LEVEL == 0, don't define UHD_FW_TRACE_LEVEL so that the C
    #code can easily detect if tracing is requested
    if(${TRACE_LEVEL} GREATER 0)
        add_definitions(-DUHD_FW_TRACE_LEVEL=${TRACE_LEVEL})
    endif(${TRACE_LEVEL} GREATER 0)
endif(TRACE_LEVEL)

########################################################################
# helper functions to build output formats
########################################################################
set(GEN_OUTPUTS_BIN_SIZE "bin_size_not_set") #set before calling
macro(GEN_OUTPUTS target pretty_name)
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
    #command to create a coe from bin
    add_custom_command(
        OUTPUT ${name}.coe DEPENDS ${name}.bin
        COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/utils/bin_to_coe.py ${name}.bin ${name}.coe
    )
    #add a top level target for output files
    add_custom_target(
        ${pretty_name} ALL DEPENDS ${name}.map ${name}.bin ${name}.ihx ${name}.dump ${name}.rom ${name}.coe
    )
endmacro(GEN_OUTPUTS)

########################################################################
# Add the subdirectories
########################################################################
add_subdirectory(lib)
add_subdirectory(x300)
add_subdirectory(n230)

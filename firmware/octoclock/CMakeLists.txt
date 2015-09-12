#
# Copyright 2014 Ettus Research LLC
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

cmake_minimum_required(VERSION 2.6)
set(CMAKE_SYSTEM_NAME Generic)

find_program(AVR_CC avr-gcc REQUIRED)
find_program(AVR_OBJCOPY avr-objcopy REQUIRED)
find_program(AVRDUDE avrdude REQUIRED)
set(CMAKE_C_COMPILER ${AVR_CC})

project(OCTOCLOCK C)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmcu=atmega128 -std=gnu99")

if(NOT DEFINED PROGRAMMER)
    SET(PROGRAMMER "avrisp2")
endif(NOT DEFINED PROGRAMMER)

if(OCTOCLOCK_DEBUG)
    message(STATUS "Debug enabled.")
    add_definitions(-DDEBUG)
endif(OCTOCLOCK_DEBUG)

include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/../../host/lib/usrp_clock
)

add_subdirectory(bootloader)
add_subdirectory(lib)
add_subdirectory(octoclock_r4)

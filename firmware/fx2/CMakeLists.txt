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

cmake_minimum_required(VERSION 2.6)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/config/Toolchain-sdcc.cmake)
project(USRP1 C)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/config/")
include(FindPythonInterp)

########################################################################
# Set toolchain to use SDCC
########################################################################
# we're doing mixed ASM and C
enable_language(ASM_SDCC)

########################################################################
# C flags and linking flags
########################################################################
add_definitions(-DHAVE_USRP2)
set(CMAKE_C_LINK_FLAGS "--code-loc 0x0000 --code-size 0x1800 --xram-loc 0x1800 --xram-size 0x0800 -Wl '-b USBDESCSEG = 0xE000'")
set(CMAKE_C_FLAGS "--no-xinit-opt")

########################################################################
# Setup precompile tools
########################################################################
set(REG_GENERATOR ${CMAKE_SOURCE_DIR}/utils/generate_regs.py)
set(EDIT_GPIF_USRP1 ${CMAKE_SOURCE_DIR}/utils/edit-gpif.py)
set(EDIT_GPIF_B100 ${CMAKE_SOURCE_DIR}/utils/edit-gpif-b100.py)
set(BUILD_EEPROM ${CMAKE_SOURCE_DIR}/utils/build_eeprom.py)

########################################################################
# Add the subdirectories
########################################################################
add_subdirectory(usrp1)
add_subdirectory(b100)

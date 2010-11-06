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

# support for the SDCC assembler, asx8051
SET( ASM_DIALECT "_SDCC" )
SET( CMAKE_ASM${ASM_DIALECT}_SOURCE_FILE_EXTENSIONS a51 )

#i don't want to talk about it. i had such high hopes for CMake.
SET( CMAKE_ASM${ASM_DIALECT}_COMPILE_OBJECT "<CMAKE_ASM${ASM_DIALECT}_COMPILER> <FLAGS> -plosgff <SOURCE>" "${CMAKE_COMMAND} -DFILE=<OBJECT> -DSOURCE=<SOURCE> -P ${CMAKE_SOURCE_DIR}/config/Rename.cmake")

INCLUDE( CMakeASMInformation )
SET( CMAKE_ASM${ASM_DIALECT}_OUTPUT_EXTENSION ".rel" ) #must go here because the include appears to overwrite it, although it shouldn't
# for future use
SET( ASM_DIALECT )

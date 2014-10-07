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
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/../../host/lib/usrp/x300)

list(APPEND x300_sources x300_init.c x300_main.c)

########################################################################
set(GEN_OUTPUTS_BIN_SIZE 0x7fff)
SET(CMAKE_C_FLAGS_DEBUG -DX300_DEBUG_UART)

add_executable(x300_main.elf ${x300_sources})
target_link_libraries(x300_main.elf usrp3fw)
GEN_OUTPUTS(x300_main.elf x300)

INSTALL(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/x300_main.bin
    DESTINATION share/uhd/images
    RENAME usrp_x300_fw.bin
)

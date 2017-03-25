#
# Copyright 2010-2014,2016 Ettus Research LLC
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
include_directories(${CMAKE_SOURCE_DIR}/../../host/lib/usrp/n230)

list(APPEND n230_sources
    n230_eeprom.c
    n230_fw_comm_protocol.c
    n230_eth_handlers.c
    n230_init.c
    n230_main.c)

########################################################################
set(GEN_OUTPUTS_BIN_SIZE 0x7fff)

add_executable(n230_main.elf ${n230_sources})
target_link_libraries(n230_main.elf usrp3fw)
GEN_OUTPUTS(n230_main.elf n230)

#INSTALL(
#    FILES ${CMAKE_CURRENT_BINARY_DIR}/n230_main.bin
#    DESTINATION share/uhd/images
#    RENAME usrp_n230_fw.bin
#)

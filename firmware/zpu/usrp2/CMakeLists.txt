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
INCLUDE(${CMAKE_SOURCE_DIR}/lib/CMakeLists.txt)
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

ADD_LIBRARY(libusrp2fw STATIC
    ${COMMON_SRCS}
    sd.c
    ethernet.c
    udp_fw_update.c
)

########################################################################
ADD_EXECUTABLE(usrp2_txrx_uhd.elf ${CMAKE_SOURCE_DIR}/apps/txrx_uhd.c)
TARGET_LINK_LIBRARIES(usrp2_txrx_uhd.elf libusrp2fw)
GEN_OUTPUTS(usrp2_txrx_uhd.elf)

ADD_EXECUTABLE(usrp2_blinkenlights.elf ${CMAKE_SOURCE_DIR}/apps/blinkenlights.c)
TARGET_LINK_LIBRARIES(usrp2_blinkenlights.elf libusrp2fw)
GEN_OUTPUTS(usrp2_blinkenlights.elf)

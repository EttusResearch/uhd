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
SET(COMMON_SRCS
    ${CMAKE_SOURCE_DIR}/lib/u2_init.c
    ${CMAKE_SOURCE_DIR}/lib/abort.c
    ${CMAKE_SOURCE_DIR}/lib/ad9510.c
    ${CMAKE_SOURCE_DIR}/lib/clocks.c
    ${CMAKE_SOURCE_DIR}/lib/eeprom.c
    ${CMAKE_SOURCE_DIR}/lib/eth_addrs.c
    ${CMAKE_SOURCE_DIR}/lib/eth_mac.c
    ${CMAKE_SOURCE_DIR}/lib/_exit.c
    ${CMAKE_SOURCE_DIR}/lib/exit.c
    ${CMAKE_SOURCE_DIR}/lib/hal_io.c
    ${CMAKE_SOURCE_DIR}/lib/hal_uart.c
    ${CMAKE_SOURCE_DIR}/lib/i2c.c
    ${CMAKE_SOURCE_DIR}/lib/mdelay.c
    ${CMAKE_SOURCE_DIR}/lib/memcpy_wa.c
    ${CMAKE_SOURCE_DIR}/lib/memset_wa.c
    ${CMAKE_SOURCE_DIR}/lib/nonstdio.c
    ${CMAKE_SOURCE_DIR}/lib/pic.c
    ${CMAKE_SOURCE_DIR}/lib/pkt_ctrl.c
    ${CMAKE_SOURCE_DIR}/lib/print_addrs.c
    ${CMAKE_SOURCE_DIR}/lib/print_rmon_regs.c
    ${CMAKE_SOURCE_DIR}/lib/print_buffer.c
    ${CMAKE_SOURCE_DIR}/lib/printf.c
    ${CMAKE_SOURCE_DIR}/lib/ihex.c
    ${CMAKE_SOURCE_DIR}/lib/spi.c
    ${CMAKE_SOURCE_DIR}/lib/net_common.c
    ${CMAKE_SOURCE_DIR}/lib/arp_cache.c
    ${CMAKE_SOURCE_DIR}/lib/banal.c
    ${CMAKE_SOURCE_DIR}/lib/udp_uart.c
)

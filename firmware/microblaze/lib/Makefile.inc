#
# Copyright 2010 Ettus Research LLC
#
# Copyright 2007 Free Software Foundation, Inc.
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

COMMON_SRCS = \
	$(top_srcdir)/lib/u2_init.c \
	$(top_srcdir)/lib/abort.c \
	$(top_srcdir)/lib/ad9510.c \
	$(top_srcdir)/lib/bsm12.c \
	$(top_srcdir)/lib/buffer_pool.c \
	$(top_srcdir)/lib/clocks.c \
	$(top_srcdir)/lib/dbsm.c \
	$(top_srcdir)/lib/eeprom.c \
	$(top_srcdir)/lib/eth_mac.c \
	$(top_srcdir)/lib/_exit.c \
	$(top_srcdir)/lib/exit.c \
	$(top_srcdir)/lib/hal_io.c \
	$(top_srcdir)/lib/hal_uart.c \
	$(top_srcdir)/lib/i2c.c \
	$(top_srcdir)/lib/mdelay.c \
	$(top_srcdir)/lib/memcpy_wa.c \
	$(top_srcdir)/lib/memset_wa.c \
	$(top_srcdir)/lib/nonstdio.c \
	$(top_srcdir)/lib/pic.c \
	$(top_srcdir)/lib/print_mac_addr.c \
	$(top_srcdir)/lib/print_rmon_regs.c \
	$(top_srcdir)/lib/print_buffer.c \
	$(top_srcdir)/lib/printf.c \
	$(top_srcdir)/lib/spi.c \
	$(top_srcdir)/lib/net_common.c \
	$(top_srcdir)/lib/arp_cache.c \
	$(top_srcdir)/lib/banal.c

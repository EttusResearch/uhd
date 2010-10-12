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

include $(top_srcdir)/Makefile.common

AM_CFLAGS = \
	$(COMMON_CFLAGS)

AM_LDFLAGS = \
	$(COMMON_LFLAGS) \
	-Wl,-defsym -Wl,_TEXT_START_ADDR=0x0050 \
	-Wl,-defsym -Wl,_STACK_SIZE=3072
	
LDADD = libusrp2.a

########################################################################
# USRP2 specific library and programs
########################################################################
noinst_LIBRARIES = libusrp2.a

libusrp2_a_SOURCES = \
	$(COMMON_SRCS) \
	sd.c \
	ethernet.c \
  udp_fw_update.c

noinst_PROGRAMS = \
	usrp2_txrx_uhd.elf

usrp2_txrx_uhd_elf_SOURCES = \
	$(top_srcdir)/apps/txrx_uhd.c

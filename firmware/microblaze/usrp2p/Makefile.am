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
	libusrp2p.a \
	-Wl,-defsym -Wl,_TEXT_START_ADDR=0x8050 \
	-Wl,-defsym -Wl,_STACK_SIZE=3072

#all of this here is to relocate the hardware vectors to somewhere normal.
COMMON_IHX_ARGS = \
	--change-section-address .vectors.sw_exception+0x8000 \
	--change-section-address .vectors.hw_exception+0x8000 \
	--change-section-address .vectors.interrupt+0x8000 \
	--change-section-address .vectors.reset+0x8000
#	$(MB_OBJCOPY) -O ihex $< $@
# the below would work if objcopy weren't written by apes
#	$(MB_OBJCOPY) -O ihex -w --change-section-address .vectors*+0x8000 $< $@
# using the below will throw away the interrupt vectors when they get relocated below 0x0000.
#	$(MB_OBJCOPY) -O ihex --change-addresses -0x8000 $< $@

########################################################################
# USRP2P specific library and programs
########################################################################
noinst_LIBRARIES = libusrp2p.a

libusrp2p_a_SOURCES = \
	$(COMMON_SRCS) \
	clocks.c \
	spif.c \
	spi_flash.c \
	spi_flash_read.c \
	bootloader_utils.c \
	ethernet.c

noinst_PROGRAMS = \
	usrp2p_txrx_uhd.elf \
  usrp2p_blinkenlights.elf \
  usrp2p_uart_flash_loader.elf

usrp2p_txrx_uhd_elf_SOURCES = \
	$(top_srcdir)/apps/txrx_uhd.c

usrp2p_blinkenlights_elf_SOURCES = \
  $(top_srcdir)/apps/blinkenlights.c

usrp2p_uart_flash_loader_elf_SOURCES = \
  $(top_srcdir)/apps/uart_flash_loader.c

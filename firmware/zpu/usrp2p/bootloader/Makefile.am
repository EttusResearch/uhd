#
# Copyright 2007,2008,2009 Free Software Foundation, Inc.
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

ROM_LINKER_SCRIPT = u2p2-rom.ld

# loads into 8K boot ram located at 0x0000_0000
AM_CFLAGS = $(COMMON_CFLAGS) -I$(top_srcdir)/usrp2p
AM_LDFLAGS = -Wl,-T,$(ROM_LINKER_SCRIPT) $(COMMON_LFLAGS) -Wl,-defsym -Wl,_STACK_SIZE=1024

EXTRA_DIST = $(ROM_LINKER_SCRIPT)

LDADD = $(top_srcdir)/usrp2p/libusrp2p.a

noinst_PROGRAMS = \
	init_bootloader.elf

init_bootloader_elf_SOURCES = init_bootloader.c

.bin.rmi:
	$(top_srcdir)/bin/bin_to_ram_macro_init.py $< $@

_generated_from_elf += \
	$(noinst_PROGRAMS:.elf=.rmi)

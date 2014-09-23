# Copyright 2013-2014 Ettus Research LLC
#
# GNU Radio is free software; you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation; either version 3, or (at your option) any later version.
#
# GNU Radio is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# GNU Radio; see the file COPYING.  If not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Boston, MA 02110-1301, USA.

INCLUDES = usrp3_regs.h uhd_dump.h

BINARIES = chdr_log

OBJECTS = uhd_dump.o

CFLAGS = -g -O0 -Wall

LIBS = -lpcap -lm

CC = cc

.PHONY: all
all: $(BINARIES)

chdr_log: uhd_dump.o chdr_log.o $(INCLUDES)
	$(CC) $(CFLAGS) -o $@ uhd_dump.o chdr_log.o  $(LIBS) $(LDFLAGS)



clean:
	rm $(BINARIES)  $(BINARIES:%=%.o) $(OBJECTS)


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

SUBDIRS = include lib test apps

########################################################################
## Handle the pkgconfig file generation
########################################################################
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = uhd.pc

EXTRA_DIST = $(srcdir)/uhd.pc.in
BUILT_SOURCES = uhd.pc
MOSTLYCLEANFILES = $(BUILT_SOURCES)

uhd.pc: $(srcdir)/uhd.pc.in Makefile
	$(SED) \
		-e 's|@prefix[@]|$(prefix)|g' \
		-e 's|@exec_prefix[@]|$(exec_prefix)|g' \
		-e 's|@libdir[@]|$(libdir)|g' \
		-e 's|@includedir[@]|$(includedir)|g' \
		-e 's|@VERSION[@]|$(VERSION)|g' \
	$< > $@

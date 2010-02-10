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

SUBDIRS =

if HAVE_UNIT_TEST

AM_CPPFLAGS = $(GENERAL_CPPFLAGS)

LDADD = \
	$(GENERAL_LDDFLAGS) \
	$(BOOST_UNIT_TEST_FRAMEWORK_LIB) \
	$(UHD_LA)

noinst_PROGRAMS = main_test

main_test_SOURCES = \
	main_test.cpp \
	addr_test.cpp \
	device_test.cpp \
	gain_handler_test.cpp \
	usrp_dboard_test.cpp \
	wax_test.cpp

TESTS = $(noinst_PROGRAMS)

endif

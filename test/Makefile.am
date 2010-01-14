#
# Copyright 2010 Ettus Research LLC
#

include $(top_srcdir)/Makefile.common

SUBDIRS =

AM_CPPFLAGS = $(GENERAL_CPPFLAGS)

LDADD = \
	$(GENERAL_LDDFLAGS) \
	$(USRP_UHD_LA)

noinst_PROGRAMS = \
	addr_test \
	wax_test

addr_test_SOURCES = addr_test.cpp
wax_test_SOURCES = wax_test.cpp

TESTS = $(noinst_PROGRAMS)

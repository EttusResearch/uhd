#
# Copyright 2010 Ettus Research LLC
#

include $(top_srcdir)/Makefile.common

SUBDIRS =

if HAVE_CPPUNIT

AM_CPPFLAGS = \
	$(GENERAL_CPPFLAGS) \
	$(CPPUNIT_CFLAGS)

LDADD = \
	$(GENERAL_LDDFLAGS) \
	$(USRP_UHD_LA) \
	$(CPPUNIT_LIBS)

noinst_PROGRAMS = cppunit_test

cppunit_test_SOURCES = \
	addr_test.cpp \
	device_test.cpp \
	cppunit_test.cpp \
	usrp_dboard_test.cpp \
	wax_test.cpp

TESTS = $(noinst_PROGRAMS)

endif

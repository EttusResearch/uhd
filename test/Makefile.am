#
# Copyright 2010 Ettus Research LLC
#

include $(top_srcdir)/Makefile.common

SUBDIRS =

if HAVE_UNIT_TEST

AM_CPPFLAGS = \
	$(GENERAL_CPPFLAGS) \
	$(CPPUNIT_CFLAGS)

LDADD = \
	$(GENERAL_LDDFLAGS) \
	$(BOOST_UNIT_TEST_FRAMEWORK_LIB) \
	$(USRP_UHD_LA)

noinst_PROGRAMS = main_test

main_test_SOURCES = \
	main_test.cpp \
	addr_test.cpp \
	device_test.cpp \
	usrp_dboard_test.cpp \
	wax_test.cpp

TESTS = $(noinst_PROGRAMS)

endif

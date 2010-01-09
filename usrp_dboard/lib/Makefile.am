#
# Copyright 2010 Ettus Research LLC
#

include $(top_srcdir)/Makefile.common

SUBDIRS =

AM_CPPFLAGS = \
	$(USRP_DBOARD_INCLUDES) \
	$(BOOST_CPPFLAGS)

noinst_LTLIBRARIES = \
	libusrp_dboard.la

libusrp_dboard_la_SOURCES = \
	interface.cpp

libusrp_dboard_la_LIBADD = \
	$(BOOST_LDFLAGS) \
	$(BOOST_THREAD_LIB)

noinst_HEADERS =

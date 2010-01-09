#
# Copyright 2010 Ettus Research LLC
#

include $(top_srcdir)/Makefile.common

SUBDIRS =

AM_CPPFLAGS = \
	$(USRP_UHD_INCLUDES) \
	$(BOOST_CPPFLAGS)

lib_LTLIBRARIES = \
	libusrp_uhd.la

libusrp_uhd_la_SOURCES = \
	usrp_addr.cpp \
	usrp_uhd.cpp \
	wax.cpp

libusrp_uhd_la_LIBADD = \
	$(BOOST_LDFLAGS) \
	$(BOOST_THREAD_LIB) \
	$(USRP_DBOARD_LA)

noinst_HEADERS =

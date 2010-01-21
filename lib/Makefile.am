#
# Copyright 2010 Ettus Research LLC
#

include $(top_srcdir)/Makefile.common

SUBDIRS = usrp quadradio

AM_CPPFLAGS = $(GENERAL_CPPFLAGS)

lib_LTLIBRARIES = libusrp_uhd.la

libusrp_uhd_la_SOURCES = \
	device.cpp \
	device_addr.cpp \
	usrp_uhd.cpp \
	wax.cpp

libusrp_uhd_la_LIBADD = \
	$(GENERAL_LDDFLAGS) \
	$(USRP_UHD_USRP_LA)

noinst_HEADERS =

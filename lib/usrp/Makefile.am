#
# Copyright 2010 Ettus Research LLC
#

include $(top_srcdir)/Makefile.common

SUBDIRS = dboard mboard

AM_CPPFLAGS = $(GENERAL_CPPFLAGS)

noinst_LTLIBRARIES = lib.la

lib_la_SOURCES = \
	usrp.cpp

lib_la_LIBADD = \
	$(GENERAL_LDDFLAGS) \
	$(USRP_UHD_USRP_DBOARD_LA) \
	$(USRP_UHD_USRP_MBOARD_LA)

#
# Copyright 2010 Ettus Research LLC
#

include $(top_srcdir)/Makefile.common

SUBDIRS =

AM_CPPFLAGS = $(GENERAL_CPPFLAGS)

noinst_LTLIBRARIES = lib.la

dboard_sources = \
	basic.cpp

lib_la_SOURCES = \
	$(dboard_sources) \
	base.cpp \
	interface.cpp \
	manager.cpp

lib_la_LIBADD = $(GENERAL_LDDFLAGS)

noinst_HEADERS = dboards.hpp

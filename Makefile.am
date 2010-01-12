#
# Copyright 2010 Ettus Research LLC
#

include $(top_srcdir)/Makefile.common

SUBDIRS = include lib test

########################################################################
## Handle the pkgconfig file generation
########################################################################
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = usrp_uhd.pc

EXTRA_DIST = $(srcdir)/usrp_uhd.pc.in
BUILT_SOURCES = usrp_uhd.pc

usrp_uhd.pc: $(srcdir)/usrp_uhd.pc.in Makefile
	$(SED) \
		-e 's|@prefix[@]|$(prefix)|g' \
		-e 's|@exec_prefix[@]|$(exec_prefix)|g' \
		-e 's|@libdir[@]|$(libdir)|g' \
		-e 's|@includedir[@]|$(includedir)|g' \
		-e 's|@VERSION[@]|$(VERSION)|g' \
	$< > $@

#
# Copyright 2010 Ettus Research LLC
#

##################################################
## Setup Autotools
##################################################
AC_PREREQ(2.60)
AC_INIT
AM_INIT_AUTOMAKE(usrp_uhd, 0)

##################################################
## Check Libs
##################################################
AC_PROG_CXX
LT_INIT
AX_BOOST_BASE([1.36])
AX_BOOST_THREAD

##################################################
## Check Programs
##################################################
AC_PATH_PROG(SED, sed)
AC_PATH_PROG(PYTHON, python)

##################################################
## Create Files
##################################################
AC_CONFIG_FILES([ \
	Makefile \
	include/Makefile \
	include/usrp_uhd/Makefile \
	include/usrp_uhd/usrp/Makefile \
	include/usrp_uhd/usrp/dboard/Makefile \
	include/usrp_uhd/usrp/mboard/Makefile \
	include/usrp_uhd/quadradio/Makefile \
	lib/Makefile \
	lib/usrp/Makefile \
	lib/usrp/dboard/Makefile \
	lib/usrp/mboard/Makefile \
	lib/quadradio/Makefile \
	test/Makefile \
])
AC_OUTPUT

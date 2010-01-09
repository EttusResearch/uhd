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
	usrp_uhd/usrp_uhd.pc \
	usrp_uhd/Makefile \
	usrp_uhd/include/Makefile \
	usrp_uhd/lib/Makefile \
	usrp_dboard/Makefile \
	usrp_dboard/include/Makefile \
	usrp_dboard/lib/Makefile \
])
AC_OUTPUT

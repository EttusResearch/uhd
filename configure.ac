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
AX_BOOST_BASE([1.36], [], AC_MSG_ERROR("cannot find boost"))
AX_BOOST_THREAD

##################################################
## Check Headers
##################################################
AC_DEFUN([UHD_CHECK_HEADER],[
    AC_CHECK_HEADER([$1], [], AC_MSG_ERROR("cannot find header $1"))
])
UHD_CHECK_HEADER([sys/uio.h])
UHD_CHECK_HEADER([arpa/inet.h])
UHD_CHECK_HEADER([netinet/ether.h])

##################################################
## Check Flags
##################################################
AC_DEFUN([UHD_OPTIONAL_CXXFLAG],[
    AX_CXX_CHECK_FLAG([$1], [], [], [CXXFLAGS="${CXXFLAGS} $1"])
])
UHD_OPTIONAL_CXXFLAG([-Wall])
UHD_OPTIONAL_CXXFLAG([-Wextra])
UHD_OPTIONAL_CXXFLAG([-Werror])
UHD_OPTIONAL_CXXFLAG([-pedantic])
UHD_OPTIONAL_CXXFLAG([-ansi])

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

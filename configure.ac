#
# Copyright 2010 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

##################################################
## Setup Autotools
##################################################
AC_PREREQ(2.60)
AC_INIT
AM_INIT_AUTOMAKE(uhd, 0)

##################################################
## Setup C++ and Libtool
##################################################
AC_PROG_CXX
LT_INIT

##################################################
## Check Boost
##################################################
AC_DEFUN([UHD_CHECK_BOOST],[$1
    AC_COMPILE_IFELSE(AC_LANG_PROGRAM([],[[
        #ifndef $2
            fail
        #endif
        return 0;
    ]]), [$3], [$4])
])

UHD_CHECK_BOOST(
    [AX_BOOST_BASE([1.36])], [HAVE_BOOST],
    [], [AC_MSG_ERROR("cannot find boost")]
)
UHD_CHECK_BOOST(
    [AX_BOOST_ASIO], [HAVE_BOOST_ASIO],
    [], [AC_MSG_ERROR("cannot find boost asio")]
)
UHD_CHECK_BOOST(
    [AX_BOOST_DATE_TIME], [HAVE_BOOST_DATE_TIME],
    [], [AC_MSG_ERROR("cannot find boost date time")]
)
UHD_CHECK_BOOST(
    [AX_BOOST_PROGRAM_OPTIONS], [HAVE_BOOST_PROGRAM_OPTIONS],
    [], [AC_MSG_ERROR("cannot find boost program options")]
)
UHD_CHECK_BOOST(
    [AX_BOOST_SYSTEM], [HAVE_BOOST_SYSTEM],
    [], [AC_MSG_ERROR("cannot find boost system")]
)
UHD_CHECK_BOOST(
    [AX_BOOST_THREAD], [HAVE_BOOST_THREAD],
    [], [AC_MSG_ERROR("cannot find boost thread")]
)
UHD_CHECK_BOOST(
    [AX_BOOST_UNIT_TEST_FRAMEWORK], [HAVE_BOOST_UNIT_TEST_FRAMEWORK],
    [HAVE_UNIT_TEST=true], [HAVE_UNIT_TEST=false]
)
AM_CONDITIONAL([HAVE_UNIT_TEST], [$HAVE_UNIT_TEST])

##################################################
## Check Headers
##################################################
AC_DEFUN([UHD_CHECK_HEADER],[
    AC_CHECK_HEADER([$1], [], [AC_MSG_ERROR("cannot find header $1")])
])

UHD_CHECK_HEADER([netinet/in.h])
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
	apps/Makefile \
	include/Makefile \
	include/uhd/Makefile \
	include/uhd/usrp/Makefile \
	include/uhd/quadradio/Makefile \
	include/uhd/transport/Makefile \
	include/uhd/usrp/dboard/Makefile \
	include/uhd/usrp/mboard/Makefile \
	lib/Makefile \
	lib/quadradio/Makefile \
	lib/transport/Makefile \
	lib/usrp/Makefile \
	lib/usrp/dboard/Makefile \
	lib/usrp/mboard/Makefile \
	test/Makefile \
])
AC_OUTPUT

dnl Copyright 2001,2002,2003,2004,2005,2006,2007,2008,2009 Free Software Foundation, Inc.
dnl 
dnl This file is part of GNU Radio
dnl 
dnl GNU Radio is free software; you can redistribute it and/or modify
dnl it under the terms of the GNU General Public License as published by
dnl the Free Software Foundation; either version 3, or (at your option)
dnl any later version.
dnl 
dnl GNU Radio is distributed in the hope that it will be useful,
dnl but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl GNU General Public License for more details.
dnl 
dnl You should have received a copy of the GNU General Public License
dnl along with GNU Radio; see the file COPYING.  If not, write to
dnl the Free Software Foundation, Inc., 51 Franklin Street,
dnl Boston, MA 02110-1301, USA.

AC_DEFUN([GRC_FX2],[
    GRC_ENABLE(usrp)

    GRC_WITH(usrp)

    dnl If execution gets to here, $passed will be:
    dnl   with : if the --with code didn't error out
    dnl   yes  : if the --enable code passed muster and all dependencies are met
    dnl   no   : otherwise
    if test $passed = yes; then
        dnl gnulib.
        dnl FIXME: this needs to fail gracefully and continue, not implemented yet
        UTILS_FUNC_MKSTEMP

        dnl These checks don't fail
        AC_C_BIGENDIAN
        AC_CHECK_HEADERS([byteswap.h linux/compiler.h])
        AC_CHECK_FUNCS([getrusage sched_setscheduler pthread_setschedparam])
        AC_CHECK_FUNCS([sigaction snprintf])

	dnl Make sure SDCC >= 2.4.0 is available.
        USRP_SDCC([2.4.0],[],[passed=no;AC_MSG_RESULT([Unable to find firmware compiler SDCC.])])
    fi
    if test $passed != with; then
	dnl how and where to find INCLUDES and LA
	usrp_INCLUDES=" \
		-I\${abs_top_srcdir}/include"
    fi

    AC_CONFIG_FILES([ \
        include/Makefile \
        lib/Makefile \
        src/Makefile \
        src/common/Makefile \
        src/usrp1/Makefile \
    ])

    GRC_BUILD_CONDITIONAL(usrp)
])

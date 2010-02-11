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
AM_INIT_AUTOMAKE(uhd-mb, 0)

##################################################
## Setup Compiler
##################################################
dnl Fix 2.64 cross compile detection for AVR and RTEMS
dnl by not trying to compile fopen.
m4_if(m4_defn([m4_PACKAGE_VERSION]), [2.64],
  [m4_foreach([_GCC_LANG], [C, C++, Fortran, Fortran 77],
     [m4_define([_AC_LANG_IO_PROGRAM(]_GCC_LANG[)], m4_defn([AC_LANG_PROGRAM(]_GCC_LANG[)]))])]) 

AC_PROG_CC([mb-gcc])
LT_INIT

##################################################
## Create Files
##################################################
AC_CONFIG_FILES([ \
    Makefile \
    apps/Makefile \
    include/Makefile \
    include/net/Makefile \
    lib/Makefile \
])
AC_OUTPUT

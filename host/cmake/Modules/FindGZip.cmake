#
# Copyright 2013 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
find_program(GZIP_EXECUTABLE NAMES gzip)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GZip DEFAULT_MSG GZIP_EXECUTABLE)

#
# Copyright 2013 Ettus Research LLC
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
FIND_PROGRAM(GZIP_EXECUTABLE NAMES gzip)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GZip DEFAULT_MSG GZIP_EXECUTABLE)

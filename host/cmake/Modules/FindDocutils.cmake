#
# Copyright 2011-2011 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
find_program(RST2HTML_EXECUTABLE NAMES rst2html rst2html.py)
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Docutils DEFAULT_MSG RST2HTML_EXECUTABLE)

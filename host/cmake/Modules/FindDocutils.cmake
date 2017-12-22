#
# Copyright 2011-2011 Ettus Research LLC
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
FIND_PROGRAM(RST2HTML_EXECUTABLE NAMES rst2html rst2html.py)
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Docutils DEFAULT_MSG RST2HTML_EXECUTABLE)

#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
#
#   This file contains the sequence of steps required to
#   build and generate outputs for the ZBX CPLD, XO3 variant.
#   It is intended to be use as a parameter for
#   pnmainc (Diamond TCL console).
#       pnmainc.exe build.tcl > <LOG_FILE_NAME>

prj_project open "zbx_top_cpld.ldf"
prj_run Synthesis -impl impl1
prj_run Translate -impl impl1
prj_run Map -impl impl1
prj_run PAR -impl impl1
prj_run PAR -impl impl1 -task PARTrace
prj_run Export -impl impl1 -task Bitgen
prj_run Export -impl impl1 -task Jedecgen
prj_run Export -impl impl1 -task Jedec4Xo3l
prj_project close

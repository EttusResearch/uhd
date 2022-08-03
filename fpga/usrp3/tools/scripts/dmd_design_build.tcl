#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
#   This is a template that will be copied to a project's
#   build directory.
#   This file contains the sequence of steps required to
#   build and generate outputs for Lattice programmables.
#   It works in conjuction with Lattice TCL Console (pnmainc).
#   This file has the pre-requisite of setting the following
#   environment variables:
#     - DMD_PROJECT_FILE  : Lattice project file name
#     - DMD_IMPL          : Implementation to use in said project
#     - DMD_GIT_HASH      : Hash to be consumed in this build
#
#   Usage:
#       pnmainc.exe build.tcl > <LOG_FILE_NAME>

prj_project open $::env(DMD_PROJECT_FILE)
prj_impl option -append "VERILOG_DIRECTIVES" $::env(DMD_GIT_HASH)
prj_run Synthesis -impl $::env(DMD_IMPL)
prj_run Translate -impl $::env(DMD_IMPL)
prj_run Map -impl $::env(DMD_IMPL)
prj_run PAR -impl $::env(DMD_IMPL)
prj_run PAR -impl $::env(DMD_IMPL) -task PARTrace
prj_run Export -impl $::env(DMD_IMPL) -task Bitgen
prj_run Export -impl $::env(DMD_IMPL) -task Jedecgen
prj_run Export -impl $::env(DMD_IMPL) -task Jedec4Xo3l
prj_project close

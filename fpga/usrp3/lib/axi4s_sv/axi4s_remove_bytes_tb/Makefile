#
# Copyright 2020 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../top)
# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/axi4s_sv/Makefile.srcs

DESIGN_SRCS = $(abspath \
$(AXI4S_SV_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
# Define only one toplevel module
TB_TOP_MODULE = axi4s_remove_bytes_all_tb
SIM_TOP = $(TB_TOP_MODULE)

SIM_SRCS = \
$(abspath $(TB_TOP_MODULE).sv )\
$(abspath axi4s_remove_bytes_tb.sv )

# supressing the following worthless reminder.
#* Warning: M:/usrp4-hw/oss-repo/fpga/usrp3/lib/axi4s_sv/axi4s_remove_bytes.sv(228): (vlog-2583) [SVCHK] -
# Extra checking for conflicts with always_comb and always_latch variables is done at vopt time
SVLOG_ARGS = -suppress 2583

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

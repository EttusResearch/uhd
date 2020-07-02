#
# Copyright 2018 Ettus Research, A National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../../top)
# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/dsp/Makefile.srcs

DESIGN_SRCS = $(abspath \
$(DSP_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
# Define toplevel module
SIM_TOP = mult_add_clip_tb glbl

# Add test bench, user design under test, and
# additional user created files
SIM_SRCS = $(abspath \
mult_add_clip_tb.sv \
$(VIVADO_PATH)/data/verilog/src/glbl.v \
)

# MODELSIM_USER_DO = $(abspath wave.do)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

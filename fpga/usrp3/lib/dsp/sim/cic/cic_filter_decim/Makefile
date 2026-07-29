#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../../../top)
# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Include makefiles and sources for the DUT and its dependencies

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = cic_filter_decim_all_tb

MODELSIM_ARGS += glbl

SIM_SRCS = \
$(abspath ../cic_test_pkg.sv) \
$(abspath ../comb_filter/cic_comb_filter_test_pkg.sv) \
$(abspath ../cic_filter_test_pkg.sv) \
$(abspath cic_filter_decim_tb.sv) \
$(abspath cic_filter_decim_all_tb.sv) \
$(abspath $(VIVADO_PATH)/data/verilog/src/glbl.v)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

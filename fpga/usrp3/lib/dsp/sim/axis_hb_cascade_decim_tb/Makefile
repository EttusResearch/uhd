#
# Copyright 2026 Ettus Research, a National Instruments Brand
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
include $(BASE_DIR)/../lib/dsp/hb_filter/Makefile.srcs

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = axis_hb_cascade_decim_all_tb

# Simulation sources
SIM_SRCS = \
$(abspath axis_hb_decim_fir_tb/axis_hb_test_pkg.sv) \
$(abspath axis_hb_cascade_decim_tb.sv) \
$(abspath axis_hb_cascade_decim_all_tb.sv) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

#-------------------------------------------------
# Debugging of Makefile
#-------------------------------------------------
# Call make print-MACRO_NAME e.g. make print-IP_DIR
print-%: ; @echo $* is $($*)

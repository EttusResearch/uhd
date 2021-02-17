#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir.
BASE_DIR = $(abspath ../../../../top)
# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------

DESIGN_SRCS += $(abspath \
$(abspath ../../utils/chdr_convert_up.v) \
$(abspath ../../utils/chdr_convert_down.v) \
$(abspath ../../utils/chdr_resize.v) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = chdr_resize_all_tb
SIM_SRCS = \
$(abspath chdr_resize_tb.sv) \
$(abspath chdr_resize_all_tb.sv) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

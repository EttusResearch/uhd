#
# Copyright 2023 Ettus Research, a National Instruments Brand
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

DESIGN_SRCS += \
$(abspath ../../../hwutils/device_dna.v) \
$(abspath ../../../hwutils/device_dna_ctrlport.v) \
$(abspath ../../../rfnoc/utils/ctrlport_reg_ro.v) \
$(VIVADO_PATH)/data/verilog/src/glbl.v \

MODELSIM_ARGS += glbl

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = device_dna_tb

SIM_SRCS = \
$(abspath device_dna_tb.sv) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

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
$(abspath ../../../sec/license_check.sv) \
$(abspath ../../../sec/sha256.v) \
$(abspath ../../../sec/sha256_core.v) \
$(abspath ../../../sec/sha256_k_constants.v) \
$(abspath ../../../sec/sha256_stream.v) \
$(abspath ../../../sec/sha256_w_mem.v) \
$(VIVADO_PATH)/data/verilog/src/glbl.v \

MODELSIM_ARGS += glbl

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = license_check_tb

SIM_SRCS = \
$(abspath license_check_tb.sv) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

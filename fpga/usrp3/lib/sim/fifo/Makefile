#
# Copyright 2026 Ettus Research, a National Instruments Brand
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
include $(BASE_DIR)/../lib/control/Makefile.srcs
include $(BASE_DIR)/../lib/fifo/Makefile.srcs

DESIGN_SRCS = $(abspath \
$(CONTROL_LIB_SRCS) \
$(FIFO_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = axi_fifo_tb_all

SIM_SRCS = \
$(abspath axi_fifo/axi_fifo_tb.sv) \
$(abspath axi_fifo_uram/axi_fifo_uram_tb.sv) \
$(abspath axi_fifo_large/axi_fifo_large_tb.sv) \
$(abspath axi_fifo_tb_all.sv) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

#
# Copyright 2019 Ettus Research, a National Instruments Brand
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
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
LIB_IP_DIR = $(BASE_DIR)/../lib/ip

# Include makefiles and sources for all IP components
# *after* defining the LIB_IP_DIR
include $(LIB_IP_DIR)/axi_fft/Makefile.inc
include $(LIB_IP_DIR)/complex_to_magphase/Makefile.inc

DESIGN_SRCS += $(abspath \
$(LIB_IP_AXI_FFT_OUTS) \
)

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/utils/Makefile.srcs
include Makefile.srcs

DESIGN_SRCS += $(abspath \
$(RFNOC_CORE_SRCS) \
$(RFNOC_UTIL_SRCS) \
$(RFNOC_OOT_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = rfnoc_block_fft_tb glbl
SIM_SRCS = \
$(abspath rfnoc_block_fft_tb.sv) \
$(VIVADO_PATH)/data/verilog/src/glbl.v \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

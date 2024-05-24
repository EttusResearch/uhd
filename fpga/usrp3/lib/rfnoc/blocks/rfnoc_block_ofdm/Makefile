#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../../top)
# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
LIB_IP_DIR = $(BASE_DIR)/../lib/ip

# Include makefiles and sources for all IP components
# *after* defining the LIB_IP_DIR
include $(LIB_IP_DIR)/xfft_32k_16b/Makefile.inc
include $(LIB_IP_DIR)/xfft_8k_16b/Makefile.inc
include $(LIB_IP_DIR)/xfft_4k_16b/Makefile.inc

DESIGN_SRCS += $(abspath \
$(LIB_IP_XFFT_32K_16B_OUTS) \
$(LIB_IP_XFFT_8K_16B_OUTS) \
$(LIB_IP_XFFT_4K_16B_OUTS) \
)

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Include makefiles and sources for the DUT and its
# dependencies.
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/utils/Makefile.srcs
include Makefile.srcs

DESIGN_SRCS += $(abspath \
$(RFNOC_CORE_SRCS) \
$(RFNOC_OOT_SRCS)  \
$(RFNOC_NI_ORU_IP_XCI_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = rfnoc_block_ofdm_tb
SIM_SRCS = $(abspath \
rfnoc_block_ofdm_tb.sv \
)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

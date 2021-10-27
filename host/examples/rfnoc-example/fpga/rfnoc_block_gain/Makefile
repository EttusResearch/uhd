#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir. Note:
# UHD_FPGA_DIR must be passed into this Makefile.
ifndef UHD_FPGA_DIR
$(error "UHD_FPGA_DIR is not set! Must point to UHD FPGA repository!")
endif
BASE_DIR = $(UHD_FPGA_DIR)/usrp3/top
# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------

# In-tree IP
LIB_IP_DIR = $(BASE_DIR)/../lib/ip
include $(LIB_IP_DIR)/complex_multiplier/Makefile.inc

# Out-of-tree IP
OOT_FPGA_DIR = $(dir $(abspath $(firstword $(MAKEFILE_LIST))))/../
include $(OOT_FPGA_DIR)/ip/cmplx_mul/Makefile.inc

# Include makefiles and sources for the DUT and its 
# dependencies.
include $(BASE_DIR)/../lib/rfnoc/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/utils/Makefile.srcs
include Makefile.srcs

DESIGN_SRCS += $(abspath          \
$(RFNOC_SRCS)                     \
$(RFNOC_CORE_SRCS)                \
$(RFNOC_UTIL_SRCS)                \
$(RFNOC_OOT_SRCS)                 \
$(LIB_IP_CMPLX_MUL_SRCS)          \
$(LIB_IP_COMPLEX_MULTIPLIER_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = rfnoc_block_gain_all_tb glbl
SIM_SRCS = \
$(abspath $(IP_BUILD_DIR)/cmplx_mul/sim/cmplx_mul.vhd) \
$(abspath $(IP_BUILD_DIR)/complex_multiplier/sim/complex_multiplier.vhd) \
$(abspath rfnoc_block_gain_tb.sv) \
$(abspath rfnoc_block_gain_all_tb.sv) \
$(VIVADO_PATH)/data/verilog/src/glbl.v \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

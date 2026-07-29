#
# Copyright 2025 Ettus Research, a National Instruments Brand
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
include $(BASE_DIR)/../lib/rfnoc/Makefile.srcs

DESIGN_SRCS += $(abspath \
$(RFNOC_CORE_SRCS) \
$(RFNOC_SRCS) \
)

#-------------------------------------------------
# IP Specific
#-------------------------------------------------

# Include makefiles and sources for all IP components
# *after* defining the IP_DIR
include $(LIB_IP_DIR)/dds_sin_cos_lut_only/Makefile.inc

DESIGN_SRCS += $(abspath \
$(LIB_IP_DDS_SIN_COS_LUT_ONLY_SRCS) \
)

#-------------------------------------------------
# ModelSim Specific
#-------------------------------------------------

modelsim vlint : DESIGN_SRCS += $(abspath \
$(IP_BUILD_DIR)/dds_sin_cos_lut_only/sim/dds_sin_cos_lut_only.vhd \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = dds_ms_all_tb

SIM_SRCS = \
$(abspath dds_ms_tb.sv) \
$(abspath $(SIM_TOP).sv) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

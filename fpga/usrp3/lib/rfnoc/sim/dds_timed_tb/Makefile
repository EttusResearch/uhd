#
# Copyright 2021 Ettus Research, A National Instruments Brand
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
$(RFNOC_SRCS) \
)

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory.
IP_DIR = $(BASE_DIR)/x300/ip

# Include makefiles and sources for all IP components
# *after* defining the IP_DIR
include $(LIB_IP_DIR)/complex_multiplier_dds/Makefile.inc
include $(LIB_IP_DIR)/dds_sin_cos_lut_only/Makefile.inc

DESIGN_SRCS += $(abspath \
$(LIB_IP_COMPLEX_MULTIPLIER_DDS_SRCS) \
$(LIB_IP_DDS_SIN_COS_LUT_ONLY_SRCS) \
)

#-------------------------------------------------
# ModelSim Specific
#-------------------------------------------------

modelsim vlint : DESIGN_SRCS += $(abspath \
$(IP_BUILD_DIR)/dds_sin_cos_lut_only/sim/dds_sin_cos_lut_only.vhd \
$(IP_BUILD_DIR)/complex_multiplier_dds/sim/complex_multiplier_dds.vhd \
)

MODELSIM_ARGS = glbl

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP ?= dds_timed_tb

SIM_SRCS = \
$(abspath $(SIM_TOP).sv) \
$(VIVADO_PATH)/data/verilog/src/glbl.v \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

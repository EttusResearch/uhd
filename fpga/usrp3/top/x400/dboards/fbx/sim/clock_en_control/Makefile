#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir.
BASE_DIR = ../../../../..
# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Define part using PART_ID (<device>/<package>/<speedgrade>)
ARCH = zynquplusRFSOC
PART_ID = xczu28dr/ffvg1517/-1/e

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/control/Makefile.srcs


DESIGN_SRCS += $(abspath \
$(FIFO_SRCS) \
$(AXI_SRCS) \
$(CONTROL_LIB_SRCS) \
$(WISHBONE_SRCS) \
)


DESIGN_SRCS += $(abspath \
../../regmap/fbx_ctrl_regmap_utils.vh \
../../regmap/clock_en_regmap_utils.vh \
../../clock_en_control.v \
)

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
IP_DIR = $(BASE_DIR)/x400/ip
LIB_IP_DIR = $(BASE_DIR)/../lib/ip

# Include makefiles and sources for all IP components
# *after* defining the IP_DIR
#
# These TBs don't use any IP yet :)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
include $(BASE_DIR)/../sim/general/Makefile.srcs

# Define only one top-level module
SIM_TOP = clock_en_control_tb glbl

# Simulation runtime in microseconds
SIM_RUNTIME_US = 100000

SIM_SRCS = \
$(abspath clock_en_control_tb.sv) \
$(VIVADO_PATH)/data/verilog/src/glbl.v \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

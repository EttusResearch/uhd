#
# Copyright 2020 Ettus Research, a National Instruments Brand
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
# Define part using PART_ID (<device>/<package>/<speedgrade>)
ARCH = kintex7
PART_ID = xc7k410t/ffg900/-2

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/fifo/Makefile.srcs
include $(BASE_DIR)/../lib/axi/Makefile.srcs
include $(BASE_DIR)/../lib/control/Makefile.srcs
include $(BASE_DIR)/../lib/io_cap_gen/Makefile.srcs

DESIGN_SRCS += $(abspath \
$(FIFO_SRCS) \
$(AXI_SRCS) \
$(CONTROL_LIB_SRCS) \
$(CAT_CAP_GEN_SRCS) \
)

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
IP_DIR = $(BASE_DIR)/e320/ip
LIB_IP_DIR = $(BASE_DIR)/../lib/ip

# Include makefiles and sources for all IP components
# *after* defining the IP_DIR
include $(IP_DIR)/fifo_short_2clk/Makefile.inc

DESIGN_SRCS += $(abspath \
$(IP_FIFO_SHORT_2CLK_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
include $(BASE_DIR)/../sim/general/Makefile.srcs
include $(BASE_DIR)/../sim/axi/Makefile.srcs

MODELSIM_LIBS += unisims_ver secureip fifo_generator_v13_2_5

# Define only one top-level module
SIM_TOP = cat_io_lvds_dual_mode_tb glbl

# Simulation runtime in microseconds
SIM_RUNTIME_US = 1000

SIM_SRCS = \
$(IP_BUILD_DIR)/fifo_short_2clk/simulation/fifo_generator_vlog_beh.v \
$(IP_BUILD_DIR)/fifo_short_2clk/sim/fifo_short_2clk.v \
$(abspath cat_io_lvds_dual_mode_tb.sv) \
$(VIVADO_PATH)/data/verilog/src/glbl.v \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

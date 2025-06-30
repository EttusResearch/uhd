#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir.
BASE_DIR = ../../../../../..
# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Define part using PART_ID (<device>/<package>/<speedgrade>)
ARCH = max10
PART_ID = 10M04SAU169I7G

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/control/Makefile.srcs

DESIGN_SRCS += $(abspath \
$(CONTROL_LIB_SRCS) \
)

DESIGN_SRCS += $(abspath \
../../regmap/obx_cpld_regs_utils.vh \
../../obx_register_endpoints.sv \
)

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
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
SIM_TOP = obx_register_endpoints_tb

# Simulation runtime in microseconds
SIM_RUNTIME_US = 10000

SIM_SRCS = \
$(abspath $(SIM_TOP).sv) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

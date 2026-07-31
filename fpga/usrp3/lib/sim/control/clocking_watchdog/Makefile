#
# Copyright 2026 Ettus Research, A National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir.
BASE_DIR = $(abspath ../../../../top)
# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------

DESIGN_SRCS += $(abspath \
$(abspath ../../../control/clocking_watchdog.sv) \
$(abspath ../../../control/clock_div.v) \
$(abspath ../../../control/synchronizer.v) \
$(abspath ../../../control/synchronizer_impl.v) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
SIM_TOP = clocking_watchdog_tb
SIM_SRCS = \
$(abspath clocking_watchdog_tb.sv) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

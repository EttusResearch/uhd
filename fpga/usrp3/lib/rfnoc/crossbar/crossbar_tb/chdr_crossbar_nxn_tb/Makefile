#
# Copyright 2015 Ettus Research LLC
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../../../top)
# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Define part using PART_ID (<device>/<package>/<speedgrade>)
ARCH = kintex7
PART_ID = xc7k410t/ffg900/-2

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/control/Makefile.srcs
include $(BASE_DIR)/../lib/fifo/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/crossbar/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs

DESIGN_SRCS = $(abspath \
$(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) \
$(RFNOC_XBAR_SRCS) \
$(RFNOC_CORE_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
# Define only one toplevel module
SIM_TOP = chdr_crossbar_nxn_all_tb

SIM_SRCS = \
$(abspath chdr_crossbar_nxn_all_tb.sv) \
$(abspath chdr_crossbar_nxn_tb.sv) \
$(abspath ../crossbar_tb.sv) \
$(abspath ../chdr_traffic_source_sim.sv) \
$(abspath ../chdr_traffic_sink_sim.sv)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

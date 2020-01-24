#
# Copyright 2016 Ettus Research
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../../top)
# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
# Define only one toplevel module
SIM_TOP = axi_rate_change_tb

# Add test bench, user design under test, and
# additional user created files
SIM_SRCS = $(abspath \
axi_rate_change_tb.sv \
$(LIB_DIR)/control/ram_2port.v \
$(LIB_DIR)/fifo/axi_packet_gate.v \
$(LIB_DIR)/rfnoc/axi_rate_change.v \
$(LIB_DIR)/rfnoc/axi_drop_partial_packet.v \
)

MODELSIM_USER_DO = $(abspath wave.do)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

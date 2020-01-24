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
SIM_TOP = axi_fifo_tb

SIM_SRCS = $(abspath \
axi_fifo_tb.sv \
$(LIB_DIR)/fifo/axi_fifo.v \
$(LIB_DIR)/fifo/axi_fifo_flop.v \
$(LIB_DIR)/fifo/axi_fifo_flop2.v \
$(LIB_DIR)/fifo/axi_fifo_short.v \
$(LIB_DIR)/fifo/axi_fifo_bram.v \
)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

#
# Copyright 2015 Ettus Research LLC
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../..)
# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Define part using PART_ID (<device>/<package>/<speedgrade>)
ARCH = zynq
PART_ID= xc7z020/clg484/-1

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/fifo/Makefile.srcs
include $(BASE_DIR)/../lib/axi/Makefile.srcs
include $(BASE_DIR)/../lib/control/Makefile.srcs

DESIGN_SRCS = $(abspath \
$(FIFO_SRCS) \
$(AXI_SRCS) \
$(CONTROL_LIB_SRCS) \
)

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
IP_DIR = ../../ip

# Include makefiles and sources for all IP components
# *after* defining the IP_DIR
include $(IP_DIR)/ddr3_16bit/Makefile.inc

DESIGN_SRCS += $(abspath \
$(IP_DDR3_16BIT_SRCS) \
)

#-------------------------------------------------
# ModelSim Specific
#-------------------------------------------------

MODELSIM_IP_SRCS = $(wildcard $(abspath \
$(IP_BUILD_DIR)/ddr3_16bit/ddr3_16bit/user_design/rtl/ddr3_16bit.v \
$(IP_BUILD_DIR)/ddr3_16bit/ddr3_16bit/user_design/rtl/ddr3_16bit_mig_sim.v \
$(IP_BUILD_DIR)/ddr3_16bit/ddr3_16bit/user_design/rtl/*/*.v \
$(VIVADO_PATH)/data/verilog/src/glbl.v \
))

MODELSIM_LIBS += \
secureip \

modelsim vlint : SIM_SRCS += $(MODELSIM_IP_SRCS)

MODELSIM_ARGS = glbl -t 1fs

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
# Define only one toplevel module
SIM_TOP = dram_test_tb

SIM_SRCS = \
$(abspath dram_test_tb.sv) \
$(IP_DDR3_16BIT_SIM_OUTS) \
$(IP_DDR3_16BIT_TG_SRCS) \
$(SIM_GENERAL_SRCS) \
$(SIM_AXI_SRCS)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

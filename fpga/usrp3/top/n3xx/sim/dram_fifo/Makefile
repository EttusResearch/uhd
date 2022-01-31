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
PART_ID = xc7z100/ffg900/-2

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
include $(IP_DIR)/ddr3_32bit/Makefile.inc
include $(IP_DIR)/axi_intercon_4x64_256_bd/Makefile.inc
include $(IP_DIR)/fifo_short_2clk/Makefile.inc
include $(IP_DIR)/fifo_4k_2clk/Makefile.inc

DESIGN_SRCS += $(abspath \
$(IP_DDR3_32BIT_SRCS) \
$(IP_AXI_INTERCON_4X64_256_SRCS) \
$(IP_AXI_INTERCON_4X64_256_BD_SRCS) \
$(IP_FIFO_4K_2CLK_SRCS) \
$(IP_FIFO_SHORT_2CLK_SRCS) \
)

#-------------------------------------------------
# ModelSim Specific
#-------------------------------------------------

MODELSIM_IP_SRCS = $(wildcard $(abspath \
$(IP_BUILD_DIR)/fifo_short_2clk/sim/fifo_short_2clk.v \
$(IP_BUILD_DIR)/axi_intercon_4x64_256_bd/sim/axi_intercon_4x64_256_bd.v \
$(IP_BUILD_DIR)/axi_intercon_4x64_256_bd/ip/*/sim/*.v \
$(IP_BUILD_DIR)/ddr3_32bit/ddr3_32bit/user_design/rtl/ddr3_32bit.v \
$(IP_BUILD_DIR)/ddr3_32bit/ddr3_32bit/user_design/rtl/ddr3_32bit_mig_sim.v \
$(IP_BUILD_DIR)/ddr3_32bit/ddr3_32bit/user_design/rtl/*/*.v \
$(VIVADO_PATH)/data/verilog/src/glbl.v \
))

MODELSIM_LIBS += \
secureip \
fifo_generator_v13_2_5 \
axi_register_slice_v2_1_24 \
axi_infrastructure_v1_1_0 \
axi_dwidth_converter_v2_1_24 \
axi_crossbar_v2_1_25 \
blk_mem_gen_v8_4_4 \
axi_data_fifo_v2_1_23 \
generic_baseblocks_v2_1_0 \

modelsim vlint : SIM_SRCS += $(MODELSIM_IP_SRCS)

MODELSIM_ARGS += glbl -t 1fs

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
# Define only one top-level module
SIM_TOP = dram_fifo_tb

SIM_SRCS += \
$(abspath dram_fifo_tb.sv) \
$(abspath axis_dram_fifo_single.sv) \
$(IP_DDR3_32BIT_SIM_OUTS) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

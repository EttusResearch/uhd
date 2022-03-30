#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
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
# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/axi4s_sv/Makefile.srcs
include $(BASE_DIR)/../lib/fifo/Makefile.srcs


# If you generate the Xilinx CORE with an AXI interface you can find Xilinx's LBUS translators here
#$(abspath ../../../build-ip/xczu28drffvg1517-1e/eth_100g_bd/eth_100g_bd/ip/eth_100g_bd_cmac_usplus_0_0/eth_100g_bd_cmac_usplus_0_0/example_design/eth_100g_bd_cmac_usplus_0_0_lbus2axis_segmented_top.v) \
#$(abspath ../../../build-ip/xczu28drffvg1517-1e/eth_100g_bd/eth_100g_bd/ip/eth_100g_bd_cmac_usplus_0_0/eth_100g_bd_cmac_usplus_0_0/example_design/eth_100g_bd_cmac_usplus_0_0_axis2lbus_segmented_top.v) \

DESIGN_SRCS = $(abspath \
$(abspath ../PkgEth100gLbus.sv) \
$(abspath ../eth_100g_axis2lbus.sv) \
$(abspath ../eth_100g_lbus2axis.sv) \
$(FIFO_SRCS) \
$(AXI4S_SV_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
MODELSIM_LIBS += secureip unimacro_ver unisims_ver xilinx_vip xpm fifo_generator_v13_2_5
MODELSIM_ARGS += glbl -t 1fs
# Define toplevel module
TB_TOP_MODULE ?= lbus_all_tb
SIM_TOP = $(TB_TOP_MODULE)

SIM_SRCS = \
$(abspath axi_lbus_tb.sv) \
$(abspath lbus_axi_tb.sv) \
$(abspath $(TB_TOP_MODULE).sv) \
$(VIVADO_PATH)/data/verilog/src/glbl.v \

# Suppressing the following worthless reminder.
#* Warning: M:/usrp4-hw/oss-repo/fpga/usrp3/lib/axi4s_sv/axi4s_remove_bytes.sv(228): (vlog-2583) [SVCHK] -
# Extra checking for conflicts with always_comb and always_latch variables is done at vopt time
SVLOG_ARGS = -suppress 2583 -keep_delta
VLOG_ARGS = -keep_delta

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

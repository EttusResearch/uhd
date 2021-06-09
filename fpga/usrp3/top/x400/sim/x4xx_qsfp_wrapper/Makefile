#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

#-------------------------------------------------
# Top-of-Makefile
#-------------------------------------------------
# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../../top)
IP_DIR = $(BASE_DIR)/x400/ip


# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak
IP_BUILD_DIR = $(BASE_DIR)/x400/build-ip/xczu28drffvg1517-1e

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Define part using PART_ID (<device>/<package>/<speedgrade>)
ARCH = zynquplusRFSOC
PART_ID = xczu28dr/ffvg1517/-1/e

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/control/Makefile.srcs
include $(BASE_DIR)/../lib/axi/Makefile.srcs
include $(BASE_DIR)/../lib/axi4_sv/Makefile.srcs
include $(BASE_DIR)/../lib/axi4s_sv/Makefile.srcs
include $(BASE_DIR)/../lib/axi4lite_sv/Makefile.srcs
include $(BASE_DIR)/../lib/packet_proc/Makefile.srcs
include $(BASE_DIR)/../lib/xge_interface/Makefile.srcs
include $(BASE_DIR)/../lib/xge/Makefile.srcs
include $(BASE_DIR)/../lib/wb_spi/Makefile.srcs
include $(BASE_DIR)/../lib/fifo/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/utils/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/xport/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/xport_sv/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/crossbar/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs
include $(BASE_DIR)/../lib/xge/Makefile.srcs
include $(IP_DIR)/Makefile.inc


IP_AXI_ETH_DMA_BD_HDL_SIM_SRCS = $(wildcard $(addprefix $(IP_BUILD_DIR)/axi_eth_dma_bd/axi_eth_dma_bd/, \
sim/axi_eth_dma_bd.v\
ip/*/sim/*.h\
ip/*/sim/*.v\
ip/*/sim/*.vhd\
ip/*/bd_0/hdl/*.v\
ip/*/bd_0/sim/*.v\
ip/*/bd_0/ip/ip_*/sim/*.v\
ip/*/bd_0/ip/ip_*/sim/*.sv\
ip/*/bd_0/ip/ip_*/sim/*.vhd\
ipshared/*/hdl/*.sv\
ipshared/*/hdl/*.v\
ipshared/*/simulation/*.v\
ipshared/*/hdl/verilog/*.v\
ipshared/*/hdl/verilog/*.svh\
ipshared/*/hdl/verilog/*.vh\
))

IP_AXI_INTERCONNECT_DMA_BD_HDL_SIM_SRCS = $(wildcard $(addprefix $(IP_BUILD_DIR)/axi_interconnect_dma_bd/axi_interconnect_dma_bd/, \
ip/*/sim/*.v\
))

TOP_SRC = \
$(abspath $(BASE_DIR)/x400/x4xx_qsfp_wrapper.sv) \
$(abspath $(BASE_DIR)/x400/x4xx_qsfp_wrapper_temp.sv) \
$(abspath $(BASE_DIR)/x400/x4xx_mgt_io_core.sv) \
$(abspath $(BASE_DIR)/x400/x4xx.v)

# Xilinx IP wants lots of libraries
MODELSIM_LIBS += secureip unimacro_ver unisims_ver xilinx_vip xpm
# Needed for the HACK_SRC, speeds up the alignment phase (still long!)
VLOG_ARGS = +define+SIM_SPEED_UP
SVLOG_ARGS = -lint +define+BUILD_100G=1
# Xilinx IP wants a second file loaded
MODELSIM_ARGS = glbl -t 1fs

DESIGN_SRCS = $(abspath \
$(AXI4_SV_SRCS) \
$(AXI4S_SV_SRCS) \
$(AXI4LITE_SV_SRCS) \
$(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) \
$(AXI_SRCS) \
$(XGE_INTERFACE_SRCS) \
$(PACKET_PROC_SRCS) \
$(RFNOC_UTIL_SRCS) \
$(RFNOC_XPORT_SRCS) \
$(RFNOC_XPORT_SV_SRCS) \
$(RFNOC_XBAR_SRCS) \
$(RFNOC_CORE_SRCS) \
$(WISHBONE_SRCS) \
$(XGE_SRCS) \
$(XGE_INTERFACE_SRCS) \
$(XGE_PCS_PMA_SRCS) \
$(IP_AXI_ETH_DMA_BD_HDL_SIM_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_BD_HDL_SIM_SRCS) \
$(IP_AXI_INTERCONNECT_ETH_HDL_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_HDL_SRCS) \
$(IP_AXI_ETH_DMA_BD_HDL_SRCS) \
$(IP_100G_HDL_SRCS) \
$(AURORA_PHY_SRCS) \
$(IP_HDL_SIM_SRCS) \
$(TOP_SRC) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
# Define only one toplevel module
TB_TOP_MODULE ?= x4xx_qsfp_wrapper_all_tb

SIM_TOP = $(TB_TOP_MODULE)

SIM_SRCS = \
$(abspath x4xx_qsfp_wrapper_tb.sv) \
$(abspath $(TB_TOP_MODULE).sv)

# Suppressing the following worthless reminder.
#* Warning: M:/usrp4-hw/oss-repo/fpga/usrp3/lib/axi4s_sv/axi4s_remove_bytes.sv(228): (vlog-2583) [SVCHK] -
# Extra checking for conflicts with always_comb and always_latch variables is done at vopt time
SVLOG_ARGS = -suppress 2583

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

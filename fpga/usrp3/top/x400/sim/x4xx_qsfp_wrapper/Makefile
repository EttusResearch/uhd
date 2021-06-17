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

# Include viv_sim_preamble after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_sim_preamble.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
# Define part using PART_ID (<device>/<package>/<speedgrade>)
ARCH = zynquplusRFSOC
PART_ID = xczu28dr/ffvg1517/-1/e

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/axi4_sv/Makefile.srcs
include $(BASE_DIR)/../lib/axi4s_sv/Makefile.srcs
include $(BASE_DIR)/../lib/xge/Makefile.srcs
include $(BASE_DIR)/../lib/xge_interface/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/utils/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/xport_sv/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/crossbar/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs

DESIGN_SRCS += $(abspath \
$(AXI4_SV_SRCS) \
$(AXI4S_SV_SRCS) \
$(XGE_SRCS) \
$(XGE_INTERFACE_SRCS) \
$(RFNOC_UTIL_SRCS) \
$(RFNOC_XPORT_SV_SRCS) \
$(RFNOC_XBAR_SRCS) \
$(RFNOC_CORE_SRCS) \
)

# Add files for the DUT
DESIGN_SRCS += $(abspath \
$(BASE_DIR)/x400/x4xx_mgt_io_core.sv \
$(BASE_DIR)/x400/x4xx_qsfp_wrapper.sv \
$(BASE_DIR)/x400/x4xx_qsfp_wrapper_temp.sv \
)

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
IP_DIR = ../../ip

# Include makefiles and sources for all IP components
# *after* defining the IP_DIR
include $(IP_DIR)/axi_interconnect_eth_bd/Makefile.inc
include $(IP_DIR)/axi_interconnect_dma_bd/Makefile.inc
include $(IP_DIR)/axi_eth_dma_bd/Makefile.inc
include $(IP_DIR)/xge_pcs_pma/Makefile.inc
include $(IP_DIR)/eth_100g_bd/Makefile.inc

DESIGN_SRCS += $(abspath \
$(IP_AXI_INTERCONNECT_ETH_HDL_SRCS) \
$(IP_AXI_INTERCONNECT_ETH_BD_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_HDL_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_BD_SRCS) \
$(IP_AXI_ETH_DMA_BD_HDL_SRCS) \
$(IP_AXI_ETH_DMA_BD_SRCS) \
$(XGE_PCS_PMA_SRCS) \
$(IP_100G_HDL_SRCS) \
$(IP_100G_BD_SRCS) \
)

#-------------------------------------------------
# ModelSim Specific
#-------------------------------------------------

# Note: ipshared/*/hdl/sc_util_*_rfs.sv needs to be compiled before the rest of
# the ipshared files.
IP_AXI_ETH_DMA_BD_HDL_SIM_SRCS = $(wildcard $(addprefix $(IP_BUILD_DIR)/axi_eth_dma_bd/axi_eth_dma_bd/, \
sim/axi_eth_dma_bd.v \
ip/*/sim/*.h \
ip/*/sim/*.v \
ip/*/sim/*.vhd \
ip/*/bd_0/hdl/*.v \
ip/*/bd_0/sim/*.v \
ip/*/bd_0/ip/ip_*/sim/*.v \
ip/*/bd_0/ip/ip_*/sim/*.sv \
ip/*/bd_0/ip/ip_*/sim/*.vhd \
ipshared/*/hdl/sc_util_*_rfs.sv \
ipshared/*/hdl/*.sv \
ipshared/*/hdl/*.v \
ipshared/*/simulation/*.v \
ipshared/*/hdl/verilog/*.v \
ipshared/*/hdl/verilog/*.svh \
ipshared/*/hdl/verilog/*.vh \
))

IP_AXI_INTERCONNECT_DMA_BD_HDL_SIM_SRCS = $(wildcard $(addprefix $(IP_BUILD_DIR)/axi_interconnect_dma_bd/axi_interconnect_dma_bd/, \
sim/*.v \
ip/*/sim/*.v \
))

MISC_IP_SIM_SRCS += \
$(VIVADO_PATH)/data/verilog/src/glbl.v \
$(abspath $(IP_BUILD_DIR)/xge_pcs_pma/model_10gbe.sv) \

DESIGN_SRCS += $(abspath \
$(IP_AXI_ETH_DMA_BD_HDL_SIM_SRCS) \
$(IP_AXI_INTERCONNECT_DMA_BD_HDL_SIM_SRCS) \
$(IP_AXI_INTERCONNECT_ETH_HDL_SIM_SRCS) \
$(IP_100G_HDL_SIM_SRCS) \
$(IP_XGE_PCS_PMA_HDL_SIM_SRCS) \
$(MISC_IP_SIM_SRCS) \
)

# Xilinx IP wants lots of libraries
MODELSIM_LIBS += secureip unimacro_ver unisims_ver xilinx_vip xpm
MODELSIM_ARGS += glbl -t 1fs
# Needed for the HACK_SRC, speeds up the alignment phase (still long!)
VLOG_ARGS += +define+SIM_SPEED_UP

# Suppressing the following worthless reminder.
#* Warning: M:/usrp4-hw/oss-repo/fpga/usrp3/lib/axi4s_sv/axi4s_remove_bytes.sv(228): (vlog-2583) [SVCHK] -
# Extra checking for conflicts with always_comb and always_latch variables is done at vopt time
SVLOG_ARGS += -suppress 2583

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
# Define only one top-level module
TB_TOP_MODULE ?= x4xx_qsfp_wrapper_all_tb

SIM_TOP = $(TB_TOP_MODULE)

SIM_SRCS = \
$(abspath x4xx_qsfp_wrapper_tb.sv) \
$(abspath $(TB_TOP_MODULE).sv) \

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

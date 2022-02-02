#
# Copyright 2020 Ettus Research, a National Instruments Brand
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
SYNTH_DUT = eth_ifc_synth_test

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/control/Makefile.srcs
include $(BASE_DIR)/../lib/axi/Makefile.srcs
include $(BASE_DIR)/../lib/axi4s_sv/Makefile.srcs
include $(BASE_DIR)/../lib/packet_proc/Makefile.srcs
include $(BASE_DIR)/../lib/xge_interface/Makefile.srcs
include $(BASE_DIR)/../lib/fifo/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/utils/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/xport/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/xport_sv/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/crossbar/Makefile.srcs
include $(BASE_DIR)/../lib/rfnoc/core/Makefile.srcs

IP_DIR = $(BASE_DIR)/x300/ip
include $(BASE_DIR)/x300/ip/fifo_short_2clk/Makefile.inc

DESIGN_SRCS = $(abspath \
eth_ifc_synth_test.sv \
$(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) \
$(AXI_SRCS) \
$(AXI4S_SV_SRCS) \
$(XGE_INTERFACE_SRCS) \
$(PACKET_PROC_SRCS) \
$(RFNOC_UTIL_SRCS) \
$(RFNOC_XPORT_SRCS) \
$(RFNOC_XPORT_SV_SRCS) \
$(RFNOC_XBAR_SRCS) \
$(RFNOC_CORE_SRCS) \
$(IP_FIFO_SHORT_2CLK_SRCS) \
)

#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
MODELSIM_LIBS += secureip unimacro_ver unisims_ver xilinx_vip xpm fifo_generator_v13_2_5
MODELSIM_ARGS += glbl -t 1fs
# Define toplevel module
TB_TOP_MODULE ?= eth_ifc_all_tb
#TB_TOP_MODULE ?= eth_ifc_synth_test
SIM_TOP = $(TB_TOP_MODULE)

SIM_SRCS = \
$(abspath $(IP_BUILD_DIR)/fifo_short_2clk/sim/fifo_short_2clk.v) \
$(abspath eth_ifc_tb.sv) \
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

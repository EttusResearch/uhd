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
include $(BASE_DIR)/../lib/xge_interface/Makefile.srcs
include $(BASE_DIR)/../lib/xge/Makefile.srcs
include $(BASE_DIR)/../lib/packet_proc/Makefile.srcs

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
include $(IP_DIR)/ten_gig_eth_pcs_pma/Makefile.inc
include $(IP_DIR)/axi64_8k_2clk_fifo/Makefile.inc
include $(IP_DIR)/axi64_4k_2clk_fifo/Makefile.inc

DESIGN_SRCS += $(abspath \
$(XGE_SRCS) \
$(XGE_INTERFACE_SRCS) \
$(IP_TEN_GIG_ETH_PCS_PMA_SRCS) \
$(IP_TEN_GIGE_PHY_XCI_SRCS) \
$(TEN_GIGE_PHY_SRCS) \
$(IP_AXI64_8K_2CLK_FIFO_SRCS) \
$(IP_AXI64_4K_2CLK_FIFO_SRCS) \
$(PACKET_PROC_SRCS) \
)
#$(IP_FIFO_SHORT_2CLK_SRCS) \


#-------------------------------------------------
# Testbench Specific
#-------------------------------------------------
#include $(BASE_DIR)/../sim/general/Makefile.srcs
#include $(BASE_DIR)/../sim/control/Makefile.srcs
#include $(BASE_DIR)/../sim/axi/Makefile.srcs

# Define only one toplevel module
SIM_TOP = ten_gig_eth_loopback_tb
# Simulation runtime in microseconds
SIM_RUNTIME_US = 30000

SIM_SRCS = \
$(abspath ten_gig_eth_loopback_tb.sv) \
$(SIM_GENERAL_SRCS) \
$(SIM_CONTROL_SRCS) \
$(SIM_AXI_SRCS)

#-------------------------------------------------
# Bottom-of-Makefile
#-------------------------------------------------
# Include all simulator specific makefiles here
# Each should define a unique target to simulate
# e.g. xsim, vsim, etc and a common "clean" target
include $(BASE_DIR)/../tools/make/viv_simulator.mak

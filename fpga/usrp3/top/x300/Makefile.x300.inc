#
# Copyright 2008-2012 Ettus Research LLC
#

##################################################
# Project Setup
##################################################
# TOP_MODULE = <Input arg>
# NAME = <Input arg>
# PART_ID = <Input arg>
# ARCH = <Input arg>

##################################################
# Include other makefiles
##################################################

BASE_DIR = $(abspath ..)
IP_DIR = $(abspath ./ip)
include $(BASE_DIR)/../tools/make/viv_design_builder.mak

include $(IP_DIR)/Makefile.inc
include coregen_dsp/Makefile.srcs
include $(LIB_DIR)/ip/Makefile.inc
include $(LIB_DIR)/hls/Makefile.inc
include $(LIB_DIR)/control/Makefile.srcs
include $(LIB_DIR)/fifo/Makefile.srcs
include $(LIB_DIR)/simple_gemac/Makefile.srcs
include $(LIB_DIR)/timing/Makefile.srcs
include $(LIB_DIR)/zpu/Makefile.srcs
include $(LIB_DIR)/wishbone/Makefile.srcs
include $(LIB_DIR)/packet_proc/Makefile.srcs
include $(LIB_DIR)/xge/Makefile.srcs
include $(LIB_DIR)/xge_interface/Makefile.srcs
include $(LIB_DIR)/dsp/Makefile.srcs
include $(LIB_DIR)/io_port2/Makefile.srcs
include $(LIB_DIR)/axi/Makefile.srcs
include $(LIB_DIR)/io_cap_gen/Makefile.srcs
include $(LIB_DIR)/rfnoc/Makefile.srcs
# For sake of convenience, we include the Makefile.srcs for DRAM FIFO, DDC, and
# DUC, Replay, and of course the radio. Any other block needs to use the
# RFNOC_OOT_MAKEFILE_SRCS variable (see below).
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_axi_ram_fifo/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_radio/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_ddc/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_duc/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_replay/Makefile.srcs
# If out-of-tree modules want to be compiled into this image, then they need to
# pass in the RFNOC_OOT_MAKEFILE_SRCS as a list of Makefile.srcs files.
# Those files need to amend the RFNOC_OOT_SRCS variable with a list of actual
# source files.
-include $(RFNOC_OOT_MAKEFILE_SRCS)

IMAGE_CORE ?= $(DEFAULT_RFNOC_IMAGE_CORE_FILE)
EDGE_FILE ?= $(DEFAULT_EDGE_FILE)

##################################################
# Sources
##################################################
TOP_SRCS = \
x300.v \
x300_pcie_int.v \
x300_core.v \
x300_sfpp_io_core.v \
x300_zpu_config.vhd \
x300_eth_interface.v \
nirio_chdr64_adapter.v \
soft_ctrl.v \
capture_ddrlvds.v \
gen_ddrlvds.v \
bus_int.v \
x300.xdc \
timing.xdc \
$(IMAGE_CORE)

ifdef BUILD_10G
SFP_SERDES_SRCS += $(XGE_SRCS) $(XGE_INTERFACE_SRCS) $(abspath x300_10ge.xdc) $(TEN_GIGE_PHY_SRCS) $(IP_TEN_GIGE_PHY_XCI_SRCS)
endif

ifdef BUILD_1G
SFP_SERDES_SRCS += $(abspath x300_1ge.xdc) $(ONE_GIGE_PHY_SRCS) $(IP_ONE_GIGE_PHY_XCI_SRCS)
endif

ifdef BUILD_AURORA
SFP_SERDES_SRCS += $(abspath x300_aurora.xdc) $(AURORA_PHY_SRCS) $(IP_AURORA_XCI_SRCS)
endif

ifdef SFP0_10GBE
SFP_SERDES_SRCS += $(abspath x300_10ge_port0.xdc)
endif

ifdef SFP1_10GBE
SFP_SERDES_SRCS += $(abspath x300_10ge_port1.xdc)
endif

ifndef NO_DRAM_FIFOS
DRAM_SRCS = $(IP_DRAM_XCI_SRCS) $(abspath x300_dram.xdc)
else
DRAM_SRCS =
endif

DESIGN_SRCS = $(abspath $(TOP_SRCS)) $(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) $(SDR_LIB_SRCS) $(SERDES_SRCS) \
$(SIMPLE_GEMAC_SRCS) $(TIMING_SRCS) $(COREGEN_DSP_SRCS) \
$(UDP_SRCS) $(EXTRAM_SRCS) \
$(ZPU_CORE_SRCS) $(WISHBONE_SRCS) \
$(SFP_SERDES_SRCS) $(IOPORT2_SRCS) \
$(PACKET_PROC_SRCS) $(DSP_SRCS) $(DRAM_SRCS) \
$(AXI_SRCS) $(CAP_GEN_GENERIC_SRCS) $(IP_XCI_SRCS) $(BD_SRCS) \
$(RFNOC_SRCS) $(LIB_IP_XCI_SRCS) \
$(RFNOC_FRAMEWORK_SRCS) \
$(RFNOC_BLOCK_DUC_SRCS) $(RFNOC_BLOCK_DDC_SRCS) $(RFNOC_BLOCK_RADIO_SRCS) \
$(RFNOC_BLOCK_AXI_RAM_FIFO_SRCS) \
$(RFNOC_BLOCK_REPLAY_SRCS) \
$(RFNOC_OOT_SRCS)

EDGE_TBL_DEF="RFNOC_EDGE_TBL_FILE=$(call RESOLVE_PATH,$(EDGE_FILE))"
IMAGE_CORE_DEF="RFNOC_IMAGE_CORE_HDR=$(DEFAULT_RFNOC_IMAGE_CORE_FILE:.v=.vh)"

##################################################
# Dependency Targets
##################################################
.SECONDEXPANSION:

VERILOG_DEFS=$(EXTRA_DEFS) $(CUSTOM_DEFS) $(GIT_HASH_VERILOG_DEF) $(EDGE_TBL_DEF) $(IMAGE_CORE_DEF)

# DESIGN_SRCS and VERILOG_DEFS must be defined
bin: .prereqs
	$(call BUILD_VIVADO_DESIGN,$(abspath ./build_x300.tcl),$(TOP_MODULE),$(ARCH),$(PART_ID))

synth: .prereqs
	$(call BUILD_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_synth.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

rtl: .prereqs
	$(call CHECK_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_check_syntax.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

viv_ip: .prereqs $$(DESIGN_SRCS) ip
	@echo "IP build for $(NAME) DONE . . ."

.PHONY: bin synth rtl viv_ip

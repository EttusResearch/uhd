#
# Copyright 2008-2012 Ettus Research LLC
#

##################################################
# Project Setup
##################################################
TOP_MODULE = <Input arg>
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
include ../n3xx/coregen_dsp/Makefile.srcs
include $(LIB_DIR)/ip/Makefile.inc
include $(LIB_DIR)/hls/Makefile.inc
include $(LIB_DIR)/control/Makefile.srcs
include $(LIB_DIR)/fifo/Makefile.srcs
include $(LIB_DIR)/axi/Makefile.srcs
include $(LIB_DIR)/xge_interface/Makefile.srcs
include $(LIB_DIR)/timing/Makefile.srcs
include $(LIB_DIR)/packet_proc/Makefile.srcs
include $(LIB_DIR)/dsp/Makefile.srcs
include $(LIB_DIR)/io_cap_gen/Makefile.srcs
include $(LIB_DIR)/rfnoc/Makefile.srcs
# For the sake of convenience, we include the Makefile.srcs for the DDC, DUC,
# radio, replay and FIFO blocks. Any other block needs to use the
# RFNOC_OOT_MAKEFILE_SRCS variable (see below).
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_radio/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_ddc/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_duc/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_replay/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_axi_ram_fifo/Makefile.srcs
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
ppsloop.v \
axi_pmu.v \
spi_slave.v

ifneq (IDLE,$(findstring IDLE, $(EXTRA_DEFS)))
TOP_SRCS += e31x.v
TOP_SRCS += e31x_core.v
TOP_SRCS += e31x_dram.v
TOP_SRCS += e310_io.v
TOP_SRCS += $(EDGE_FILE) $(IMAGE_CORE)
MB_XDC = \
e31x_pins.xdc \
e31x_timing.xdc
else
TOP_SRCS += e31x_idle.v
MB_XDC = \
e31x_idle_pins.xdc
endif

DRAM_SRCS = $(IP_DRAM_XCI_SRCS)

# The XDC files must be read in a specific order, motherboard first and then daughterboard.
# Outside of that, all the other sources can be read in any order desired.
DESIGN_SRCS = \
$(abspath $(TOP_SRCS)) \
$(CONTROL_LIB_SRCS) \
$(IP_XCI_SRCS) \
$(XGE_INTERFACE_SRCS) \
$(PACKET_PROC_SRCS) \
$(AXI_SRCS) \
$(FIFO_SRCS) \
$(AURORA_PHY_SRCS) \
$(BD_SRCS) \
$(RADIO_SRCS) \
$(RFNOC_SRCS) \
$(TIMING_SRCS) \
$(VITA_SRCS) \
$(CAT_CAP_GEN_SRCS) \
$(DRAM_SRCS) \
$(COREGEN_DSP_SRCS) \
$(DSP_SRCS) \
$(LIB_IP_XCI_SRCS) \
$(LIB_HLS_IP_SRCS) \
$(EXTRAM_SRCS) \
$(CAP_GEN_GENERIC_SRCS) \
$(RFNOC_OOT_SRCS)\
$(RFNOC_FRAMEWORK_SRCS) $(RFNOC_BLOCK_NULL_SRC_SINK_SRCS) \
$(RFNOC_BLOCK_DUC_SRCS) $(RFNOC_BLOCK_DDC_SRCS) $(RFNOC_BLOCK_RADIO_SRCS) \
$(RFNOC_BLOCK_AXI_RAM_FIFO_SRCS) \
$(RFNOC_BLOCK_REPLAY_SRCS) \
$(RFNOC_BLOCK_EXAMPLE_SRCS) \
$(abspath $(MB_XDC))

EDGE_TBL_DEF="RFNOC_EDGE_TBL_FILE=$(call RESOLVE_PATH,$(EDGE_FILE))"
IMAGE_CORE_DEF="RFNOC_IMAGE_CORE_HDR=$(call RESOLVE_PATH,$(IMAGE_CORE:.v=.vh))"

##################################################
# Dependency Targets
##################################################
.SECONDEXPANSION:

VERILOG_DEFS=$(EXTRA_DEFS) $(CUSTOM_DEFS) $(GIT_HASH_VERILOG_DEF) $(EDGE_TBL_DEF) $(IMAGE_CORE_DEF)

# DESIGN_SRCS and VERILOG_DEFS must be defined
bin: .prereqs $$(DESIGN_SRCS) ip
	$(call BUILD_VIVADO_DESIGN,$(abspath ./build_e31x.tcl),$(TOP_MODULE),$(ARCH),$(PART_ID))

synth: .prereqs $$(DESIGN_SRCS) ip
	$(call BUILD_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_synth.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

rtl: .prereqs $$(DESIGN_SRCS) ip
	$(call CHECK_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_check_syntax.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

.PHONY: bin rtl

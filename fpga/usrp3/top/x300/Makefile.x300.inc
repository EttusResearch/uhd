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
include $(LIB_DIR)/axi4s_sv/Makefile.srcs
include $(LIB_DIR)/rfnoc/xport_sv/Makefile.srcs

# If out-of-tree modules want to be compiled into this image, then they need to
# pass in the RFNOC_OOT_MAKEFILE_SRCS as a list of Makefile.srcs files.
# Those files need to amend the RFNOC_OOT_SRCS variable with a list of actual
# source files.
-include $(RFNOC_OOT_MAKEFILE_SRCS)

ifdef BUILD_DIR
include $(BUILD_DIR)/Makefile.inc
endif

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
timing.xdc

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

# The XDC files must be read in a specific order, motherboard (TOP_SRCS) first
# and then SFP (SFP_SERDES_SRCS) and DRAM (DRAM_SRCS).
DESIGN_SRCS = $(abspath $(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) $(SDR_LIB_SRCS) $(SERDES_SRCS) \
$(SIMPLE_GEMAC_SRCS) $(TIMING_SRCS) $(COREGEN_DSP_SRCS) \
$(UDP_SRCS) $(EXTRAM_SRCS) \
$(ZPU_CORE_SRCS) $(WISHBONE_SRCS) \
$(IOPORT2_SRCS) \
$(PACKET_PROC_SRCS) $(DSP_SRCS) \
$(AXI_SRCS) $(AXI4S_SV_SRCS) $(CAP_GEN_GENERIC_SRCS) $(IP_XCI_SRCS) $(BD_SRCS) \
$(LIB_IP_XCI_SRCS) \
$(RFNOC_FRAMEWORK_SRCS) $(RFNOC_SRCS) $(RFNOC_OOT_SRCS) \
$(RFNOC_XPORT_SV_SRCS) \
$(RFNOC_IMAGE_CORE_SRCS) \
$(TOP_SRCS) $(SFP_SERDES_SRCS) $(DRAM_SRCS) \
)

# Pass the image core header files required by RFNoC
# to Vivado as Verilog definitions.
IMAGE_CORE_HEADER_DEF="RFNOC_IMAGE_CORE_HDR=$(BUILD_DIR)/rfnoc_image_core.vh"

##################################################
# Dependency Targets
##################################################
.SECONDEXPANSION:

VERILOG_DEFS=$(EXTRA_DEFS) $(CUSTOM_DEFS) $(GIT_HASH_VERILOG_DEF) $(IMAGE_CORE_HEADER_DEF) $(RFNOC_IMAGE_CORE_DEFS)

# DESIGN_SRCS and VERILOG_DEFS must be defined
bin: .prereqs
	@echo "Build directory:: $(BUILD_DIR)"
	@echo "Printing MB_XDC:: $(MB_XDC)"
	@echo "Printing VERILOG_DEFS:: $(VERILOG_DEFS)"
	$(call BUILD_VIVADO_DESIGN,$(abspath ./build_x300.tcl),$(TOP_MODULE),$(ARCH),$(PART_ID))

synth: .prereqs
	$(call BUILD_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_synth.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

rtl: .prereqs
	$(call CHECK_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_check_syntax.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

viv_ip: .prereqs $$(DESIGN_SRCS) ip
	@echo "IP build for $(NAME) DONE . . ."

.PHONY: bin synth rtl viv_ip

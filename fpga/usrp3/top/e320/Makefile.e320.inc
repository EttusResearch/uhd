#
# Copyright 2018-2019 Ettus Research, a National Instruments Brand
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
include ../n3xx/coregen_dsp/Makefile.srcs
include $(LIB_DIR)/ip/Makefile.inc
include $(LIB_DIR)/hls/Makefile.inc
include $(LIB_DIR)/control/Makefile.srcs
include $(LIB_DIR)/fifo/Makefile.srcs
include $(LIB_DIR)/simple_gemac/Makefile.srcs
include $(LIB_DIR)/axi/Makefile.srcs
include $(LIB_DIR)/timing/Makefile.srcs
include $(LIB_DIR)/packet_proc/Makefile.srcs
include $(LIB_DIR)/xge/Makefile.srcs
include $(LIB_DIR)/xge_interface/Makefile.srcs
include $(LIB_DIR)/dsp/Makefile.srcs
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
e320.v \
e320_core.v \
e320_clocking.v \
e320_sfp_wrapper.v \
e320_mgt_io_core.v \
$(IMAGE_CORE)

MB_XDC = \
mb_pins.xdc \
mb_timing.xdc

ifdef BUILD_10G
MB_XDC += $(abspath e320_10ge.xdc)
endif

ifdef BUILD_1G
MB_XDC += $(abspath e320_1ge.xdc)
endif

ifdef BUILD_AURORA
MB_XDC += $(abspath e320_aurora.xdc)
endif

ifdef SFP_10GBE
MB_XDC += $(abspath e320_10ge_port0.xdc)
endif

ifndef NO_DRAM_FIFOS
DRAM_SRCS = $(IP_DRAM_XCI_SRCS) $(abspath e320_dram.xdc)
else
DRAM_SRCS =
endif

DESIGN_SRCS = $(abspath \
$(CONTROL_LIB_SRCS) \
$(IP_XCI_SRCS) \
$(TEN_GIGE_PHY_SRCS) \
$(XGE_SRCS) \
$(XGE_INTERFACE_SRCS) \
$(PACKET_PROC_SRCS) \
$(AXI_SRCS) \
$(AXI4S_SV_SRCS) \
$(FIFO_SRCS) \
$(ONE_GIGE_PHY_SRCS) \
$(SIMPLE_GEMAC_SRCS) \
$(AURORA_PHY_SRCS) \
$(BD_SRCS) \
$(TIMING_SRCS) \
$(CAT_CAP_GEN_SRCS) \
$(DRAM_SRCS) \
$(COREGEN_DSP_SRCS) \
$(DSP_SRCS) \
$(LIB_IP_XCI_SRCS) \
$(LIB_HLS_IP_SRCS) \
$(EXTRAM_SRCS) \
$(CAP_GEN_GENERIC_SRCS) \
$(RFNOC_FRAMEWORK_SRCS) \
$(RFNOC_SRCS) \
$(RFNOC_OOT_SRCS) \
$(RFNOC_XPORT_SV_SRCS) \
$(RFNOC_IMAGE_CORE_SRCS) \
$(TOP_SRCS) \
$(MB_XDC) \
)

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
	$(call BUILD_VIVADO_DESIGN,$(abspath ./build_e320.tcl),$(TOP_MODULE),$(ARCH),$(PART_ID))

synth: .prereqs
	$(call BUILD_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_synth.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

rtl: .prereqs
	$(call CHECK_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_check_syntax.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

viv_ip: .prereqs $$(DESIGN_SRCS) ip
	@echo "IP build for $(NAME) DONE . . ."

.PHONY: bin synth rtl viv_ip

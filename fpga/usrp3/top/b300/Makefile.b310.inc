#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
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
include ../x300/coregen_dsp/Makefile.srcs
include $(LIB_DIR)/ip/Makefile.inc
include $(LIB_DIR)/hls/Makefile.inc
include $(LIB_DIR)/control/Makefile.srcs
include $(LIB_DIR)/fifo/Makefile.srcs
include $(LIB_DIR)/timing/Makefile.srcs
include $(LIB_DIR)/wishbone/Makefile.srcs
include $(LIB_DIR)/packet_proc/Makefile.srcs
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
b310.sv \
netlists/b310_jesd204b.vhd \
netlists/b310_jesd204b.edf \
netlists/cpld_interface_wrapper.vhd \
netlists/cpld_interface_wrapper.edf \
netlists/b310_g2x4_host_interface.vhd \
netlists/b310_g2x4_host_interface.edf \
b310_pcie_int.sv \
core/core_regs.sv \
core/basic_regs.sv \
core/radio_control_regs.sv \
core/b310_core.sv \
b3xx_pps_sync.sv \
b310_xadc_wrapper.sv \
constraints/b310.xdc \
constraints/timing.xdc \
constraints/b310_dram.xdc \
../../lib/vivado_ipi/axi_bitq/bitq_fsm.vhd \


DESIGN_SRCS = $(abspath $(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) \
$(TIMING_SRCS) $(COREGEN_DSP_SRCS) \
$(EXTRAM_SRCS) \
$(WISHBONE_SRCS) \
$(IOPORT2_SRCS) \
$(RFNOC_FRAMEWORK_SRCS) $(PACKET_PROC_SRCS) $(DSP_SRCS) \
$(AXI_SRCS) $(AXI4S_SV_SRCS) $(CAP_GEN_GENERIC_SRCS) $(IP_XCI_SRCS) $(BD_SRCS) \
$(LIB_IP_XCI_SRCS) $(PCI_K7_SRCS) \
$(RFNOC_SRCS) $(RFNOC_OOT_SRCS) \
$(RFNOC_XPORT_SV_SRCS) \
$(RFNOC_IMAGE_CORE_SRCS) \
$(TOP_SRCS)  \
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
	$(call BUILD_VIVADO_DESIGN,$(abspath ./build_b310.tcl),$(TOP_MODULE),$(ARCH),$(PART_ID))

synth: .prereqs
	$(call BUILD_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_synth.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

rtl: .prereqs
	$(call CHECK_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_check_syntax.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

viv_ip: .prereqs $$(DESIGN_SRCS) ip
	@echo "IP build for $(NAME) DONE . . ."

.PHONY: bin synth rtl viv_ip

#
# Copyright 2021 Ettus Research, a National Instruments Brand
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

# Include IP directory
include $(IP_DIR)/Makefile.inc

# Include any LIB dependencies
include coregen_dsp/Makefile.srcs
include $(LIB_DIR)/control/Makefile.srcs
include $(LIB_DIR)/fifo/Makefile.srcs
include $(LIB_DIR)/xge/Makefile.srcs
include $(LIB_DIR)/xge_interface/Makefile.srcs
include $(LIB_DIR)/axi/Makefile.srcs
include $(LIB_DIR)/packet_proc/Makefile.srcs
include $(LIB_DIR)/dsp/Makefile.srcs
include $(LIB_DIR)/ip/Makefile.inc
include $(LIB_DIR)/rfnoc/Makefile.srcs
include $(LIB_DIR)/wb_spi/Makefile.srcs
include $(LIB_DIR)/axi4_sv/Makefile.srcs
include $(LIB_DIR)/axi4s_sv/Makefile.srcs
include $(LIB_DIR)/axi4lite_sv/Makefile.srcs
include $(LIB_DIR)/rfnoc/xport_sv/Makefile.srcs

# For sake of convenience, we include the Makefile.srcs for DRAM FIFO, DDC, and
# DUC, Replay, and of course the radio. Any other block needs to use the
# RFNOC_OOT_MAKEFILE_SRCS variable (see below).
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_null_src_sink/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_radio/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_ddc/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_duc/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_axi_ram_fifo/Makefile.srcs
include $(LIB_DIR)/rfnoc/blocks/rfnoc_block_replay/Makefile.srcs

# If out-of-tree modules want to be compiled into this image, then they need to
# pass in the RFNOC_OOT_MAKEFILE_SRCS as a list of Makefile.srcs files.
# Those files need to amend the RFNOC_OOT_SRCS variable with a list of actual
# source files.
include $(RFNOC_OOT_MAKEFILE_SRCS)

IMAGE_CORE ?= $(DEFAULT_RFNOC_IMAGE_CORE_FILE)
EDGE_FILE ?= $(DEFAULT_EDGE_FILE)

##################################################
# Sources
##################################################
TOP_SRCS =

ifdef X410
TOP_SRCS += \
x4xx.v \
x4xx_qsfp_wrapper_temp.sv \
x4xx_qsfp_wrapper.sv \
x4xx_mgt_io_core.sv \
x4xx_core.v \
x4xx_core_common.v \
x4xx_global_regs.v \
x4xx_versioning_regs.v \
x4xx_dio.v \
x4xx_gpio_atr.v \
x4xx_gpio_spi.v \
x4xx_dram.v \
rf/100m/rf_core_100m.v \
rf/200m/rf_core_200m.v \
rf/200m/rf_down_4to2.v \
rf/200m/rf_up_2to4.v \
rf/400m/rf_core_400m.v \
rf/common/rf_reset_controller.vhd \
ctrlport_spi_master.v \
cpld_interface.v \
cpld_interface_regs.v \
qsfp_led_controller.v \
rfdc_timing_control.v \
x4xx_pps_sync.v \
../../lib/timing/pps_generator.v \
dboards/ctrlport_byte_serializer.v \
dboards/db_gpio_interface.v \
dboards/db_gpio_reordering.v \
ipass_present_controller.v \
$(IMAGE_CORE)
endif

MB_XDC = \
constraints/pins/common.xdc \
constraints/pins/rfdc_2x2.xdc \
constraints/timing/shared_constants.sdc \
constraints/timing/common.xdc \
constraints/pins/dram.xdc \
constraints/timing/dram.xdc

# Definitions
# MGT Types from x4xx_mgt_type.vh
MGT_100GbE   = 5
MGT_Aurora   = 3
MGT_10GbE    = 2
MGT_Disabled = 0

# Check if any of the ports are set to 10Gbe
ifneq (,$(findstring =$(MGT_10GbE),$(EXTRA_DEFS)))
  MB_XDC += constraints/timing/qsfp_10gbe.xdc
endif

# Add pin constraints for QSFP pins
ifneq (,$(findstring QSFP0_0,"$(EXTRA_DEFS)"))
  MB_XDC += constraints/pins/qsfp0_0.xdc
endif
ifneq (,$(findstring QSFP0_1,$(EXTRA_DEFS)))
  MB_XDC += constraints/pins/qsfp0_1.xdc
endif
ifneq (,$(findstring QSFP0_2,$(EXTRA_DEFS)))
  MB_XDC += constraints/pins/qsfp0_2.xdc
endif
ifneq (,$(findstring QSFP0_3,$(EXTRA_DEFS)))
  MB_XDC += constraints/pins/qsfp0_3.xdc
endif

ifneq (,$(findstring QSFP1_0,$(EXTRA_DEFS)))
  MB_XDC += constraints/pins/qsfp1_0.xdc
endif
ifneq (,$(findstring QSFP1_1,$(EXTRA_DEFS)))
  MB_XDC += constraints/pins/qsfp1_1.xdc
endif
ifneq (,$(findstring QSFP1_2,$(EXTRA_DEFS)))
  MB_XDC += constraints/pins/qsfp1_2.xdc
endif
ifneq (,$(findstring QSFP1_3,$(EXTRA_DEFS)))
  MB_XDC += constraints/pins/qsfp1_3.xdc
endif

ifdef X410
DESIGN_SRCS = $(abspath $(MB_XDC))
endif

# The XDC files must be read in a specific order, motherboard first and then daughterboard.
# Outside of that, all the other sources can be read in any order desired.
DESIGN_SRCS += $(abspath \
$(AXI4_SV_SRCS) $(AXI4S_SV_SRCS) $(AXI4LITE_SV_SRCS) $(TOP_SRCS) \
$(AXI_SRCS) $(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) $(SDR_LIB_SRCS) $(SERDES_SRCS) \
$(SIMPLE_GEMAC_SRCS) $(TIMING_SRCS) $(COREGEN_DSP_SRCS) \
$(VRT_SRCS) $(UDP_SRCS) $(COREGEN_SRCS) $(EXTRAM_SRCS) $(WISHBONE_SRCS) \
$(XGE_SRCS) $(XGE_INTERFACE_SRCS) $(AURORA_PHY_SRCS) \
$(XGE_PCS_PMA_SRCS) \
$(PACKET_PROC_SRCS) $(VITA_SRCS) $(DSP_SRCS) $(DRAM_SRCS) \
$(RADIO_SRCS) $(CAP_GEN_GENERIC_SRCS) $(IP_XCI_SRCS) $(BD_SRCS) \
$(RFNOC_SRCS) $(RFNOC_OOT_SRCS) $(LIB_IP_XCI_SRCS) $(LIB_HLS_IP_SRCS) $(LIBCOREGEN_SRCS) \
$(RFNOC_BLOCK_DUC_SRCS) $(RFNOC_BLOCK_DDC_SRCS) \
$(RFNOC_BLOCK_RADIO_SRCS) $(RFNOC_BLOCK_NULL_SRC_SINK_SRCS) \
$(RFNOC_BLOCK_AXI_RAM_FIFO_SRCS) \
$(RFNOC_BLOCK_REPLAY_SRCS) \
$(DB_COMMON_SRCS) $(WHITE_RABBIT_SRCS) $(RFNOC_FRAMEWORK_SRCS) \
$(WB_SPI_SRCS) $(RFNOC_XPORT_SV_SRCS) \
)

# Pass the edge table and image core header files required by RFNoC
# to Vivado as Verilog definitions.
EDGE_TBL_DEF="RFNOC_EDGE_TBL_FILE=$(call RESOLVE_PATH,$(EDGE_FILE))"
IMAGE_CORE_DEF="RFNOC_IMAGE_CORE_HDR=$(call RESOLVE_PATH,$(IMAGE_CORE:.v=.vh))"

##################################################
# Dependency Targets
##################################################
.SECONDEXPANSION:

VERILOG_DEFS=$(EXTRA_DEFS) $(CUSTOM_DEFS) $(GIT_HASH_VERILOG_DEF) $(EDGE_TBL_DEF) $(IMAGE_CORE_DEF)

# DESIGN_SRCS and VERILOG_DEFS must be defined
bin: .prereqs
	@echo "Printing MB_XDC:: $(MB_XDC)"
	@echo "Printing EXTRA_DEFS:: $(EXTRA_DEFS)"
	$(call BUILD_VIVADO_DESIGN,$(abspath ./build_x4xx.tcl),$(TOP_MODULE),$(ARCH),$(PART_ID))

synth: .prereqs
	$(call BUILD_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_synth.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

rtl: .prereqs
	$(call CHECK_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_check_syntax.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

viv_ip: .prereqs $$(DESIGN_SRCS) ip
	@echo "IP build for $(NAME) DONE . . ."

.PHONY: bin synth rtl viv_ip

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
include $(LIB_DIR)/wishbone/Makefile.srcs
include $(LIB_DIR)/axi4_sv/Makefile.srcs
include $(LIB_DIR)/axi4s_sv/Makefile.srcs
include $(LIB_DIR)/axi4lite_sv/Makefile.srcs
include $(LIB_DIR)/rfnoc/xport_sv/Makefile.srcs

# If out-of-tree modules want to be compiled into this image, then they need to
# pass in the RFNOC_OOT_MAKEFILE_SRCS as a list of Makefile.srcs files.
# Those files need to amend the RFNOC_OOT_SRCS variable with a list of actual
# source files.
-include $(RFNOC_OOT_MAKEFILE_SRCS)
# Now include everything that falls out of the image builder
ifdef BUILD_DIR
include $(BUILD_DIR)/Makefile.inc
endif

##################################################
# Sources
##################################################
TOP_SRCS = \
x4xx.sv \
x4xx_core.v \
x4xx_core_common.v \
x4xx_global_regs.v \
x4xx_versioning_regs.v \
x4xx_dio.v \
x4xx_gpio_atr.v \
x4xx_gpio_spi.v \
x4xx_dram.v \
ctrlport_spi_master.v \
cpld_interface.v \
cpld_interface_regs.v \
qsfp_led_controller.v \
rfdc_timing_control.sv \
x4xx_pps_sync.v \
../../lib/timing/pps_generator.v \
dboards/ctrlport_byte_serializer.v \
dboards/ctrlport_clk_crossing_derived.v \
dboards/db_gpio_reordering.v \
ipass_present_controller.v

ifdef X410
TOP_SRCS += \
rf/100m/rf_core_100m.v \
rf/200m/rf_core_200m.v \
rf/200m/rf_down_4to2.v \
rf/200m/rf_up_2to4.v \
rf/400m/rf_core_400m.v \
rf/x410/x410_rf_reset_controller.vhd \
dboards/zbx/db_gpio_interface.v
endif

ifdef X440
TOP_SRCS += \
rf/100m/rf_core_100m.v \
rf/full/rf_core_full.sv \
rf/common/adc_iq_repacker.v \
rf/x440/x440_rf_reset_controller.vhd \
dboards/fbx/db_gpio_interface.v \
dboards/fbx/ctrlport_to_i2c_sync_ctrl.v \
dboards/fbx/led_atr_control.v \
dboards/fbx/clock_en_control.v \
dboards/fbx/rf_atr_control.v
endif

ifdef X410
MB_XDC += \
constraints/timing/x410_clocks.xdc
endif

ifdef X440
ifneq (,$(findstring RF_BW=400,$(RFNOC_IMAGE_CORE_DEFS)))
MB_XDC += \
constraints/timing/x440_clocks_400.xdc
endif
ifneq (,$(findstring RF_BW=200,$(RFNOC_IMAGE_CORE_DEFS)))
MB_XDC += \
constraints/timing/x440_clocks_200.xdc
endif
MB_XDC += \
constraints/timing/x440_clocks.xdc
endif

MB_XDC += \
constraints/pins/common.xdc \
constraints/timing/shared_constants.sdc \
constraints/timing/common.xdc \
constraints/pins/dram.xdc \
constraints/timing/dram.xdc

ifdef X410
MB_XDC += \
constraints/pins/rfdc_2x2.xdc \
constraints/pins/x410/db_gpio.xdc \
constraints/timing/x410.xdc
endif

ifdef X440
MB_XDC += \
constraints/pins/rfdc_4x4.xdc \
constraints/pins/x440/db_gpio.xdc \
constraints/timing/x440.xdc
endif

MB_XDC += $(RFNOC_XDC_SRCS)

DESIGN_SRCS = $(abspath $(MB_XDC))

DESIGN_SRCS += $(abspath \
$(AXI4_SV_SRCS) $(AXI4S_SV_SRCS) $(AXI4LITE_SV_SRCS) \
$(AXI_SRCS) $(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) $(SDR_LIB_SRCS) $(SERDES_SRCS) \
$(SIMPLE_GEMAC_SRCS) $(TIMING_SRCS) $(COREGEN_DSP_SRCS) \
$(VRT_SRCS) $(UDP_SRCS) $(COREGEN_SRCS) $(EXTRAM_SRCS) $(WISHBONE_SRCS) \
$(XGE_SRCS) $(XGE_INTERFACE_SRCS) $(AURORA_PHY_SRCS) \
$(XGE_PCS_PMA_SRCS) \
$(PACKET_PROC_SRCS) $(VITA_SRCS) $(DSP_SRCS) $(DRAM_SRCS) \
$(RADIO_SRCS) $(CAP_GEN_GENERIC_SRCS) $(IP_XCI_SRCS) $(BD_SRCS) \
$(LIB_IP_XCI_SRCS) $(LIB_HLS_IP_SRCS) $(LIBCOREGEN_SRCS) $(WB_SPI_SRCS) \
$(RFNOC_FRAMEWORK_SRCS) $(RFNOC_SRCS) $(RFNOC_OOT_SRCS) \
$(RFNOC_XPORT_SV_SRCS) $(RFNOC_IMAGE_CORE_SRCS) \
$(DB_COMMON_SRCS) $(TOP_SRCS) \
)

# Pass the image core header files required by RFNoC
# to Vivado as Verilog definitions.
IMAGE_CORE_HEADER_DEF="RFNOC_IMAGE_CORE_HDR=$(BUILD_DIR)/rfnoc_image_core.vh"

##################################################
# Dependency Targets
##################################################
.SECONDEXPANSION:

VERILOG_DEFS=$(EXTRA_DEFS) $(CUSTOM_DEFS) $(GIT_HASH_VERILOG_DEF) $(IMAGE_CORE_HEADER_DEF) $(RFNOC_IMAGE_CORE_DEFS)

show-var:
	@echo "IP_BUILD_DIR: $(IP_BUILD_DIR)"

# DESIGN_SRCS and VERILOG_DEFS must be defined
bin: .prereqs
	@echo "Build directory:: $(BUILD_DIR)"
	@echo "Printing MB_XDC:: $(MB_XDC)"
	@echo "Printing VERILOG_DEFS:: $(VERILOG_DEFS)"
	$(call BUILD_VIVADO_DESIGN,$(abspath ./build_x4xx.tcl),$(TOP_MODULE),$(ARCH),$(PART_ID))

synth: .prereqs
	$(call BUILD_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_synth.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

rtl: .prereqs
	$(call CHECK_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_check_syntax.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

viv_ip: show-var .prereqs $$(DESIGN_SRCS) ip
	@echo "IP build for $(NAME) DONE . . ."

secure_core: .prereqs
	$(call BUILD_VIVADO_DESIGN,$(TOOLS_DIR)/scripts/viv_secure_synth.tcl,$(TOP_MODULE),$(ARCH),$(PART_ID))

.PHONY: bin synth rtl viv_ip secure_synth_netlist

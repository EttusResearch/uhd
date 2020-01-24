#
# Copyright 2015 Ettus Research LLC
#

# Define BASE_DIR to point to the "top" dir
BASE_DIR = $(abspath ../../../top)
# Include viv_sim_preample after defining BASE_DIR
include $(BASE_DIR)/../tools/make/viv_design_builder.mak

#-------------------------------------------------
# Design Specific
#-------------------------------------------------
ifndef PRODUCT
PRODUCT=X310
endif

ARCH=$(XIL_ARCH_${PRODUCT})
PART_ID=$(XIL_PART_ID_${PRODUCT})

# Include makefiles and sources for the DUT and its dependencies
include $(LIB_DIR)/control/Makefile.srcs
include $(LIB_DIR)/fifo/Makefile.srcs
include $(LIB_DIR)/timing/Makefile.srcs
include $(LIB_DIR)/packet_proc/Makefile.srcs
include $(LIB_DIR)/vita/Makefile.srcs
include $(LIB_DIR)/dsp/Makefile.srcs
include $(LIB_DIR)/axi/Makefile.srcs
include $(LIB_DIR)/radio/Makefile.srcs
include $(BASE_DIR)/x300/coregen_dsp/Makefile.srcs

DESIGN_SRCS = $(abspath \
$(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) \
$(TIMING_SRCS) \
$(PACKET_PROC_SRCS) \
$(VITA_SRCS) \
$(DSP_SRCS) \
$(AXI_SRCS) \
$(RADIO_SRCS) \
$(COREGEN_DSP_SRCS) \
)

TOP_MODULE = radio

#-------------------------------------------------
# IP Specific
#-------------------------------------------------
# If simulation contains IP, define the IP_DIR and point
# it to the base level IP directory
IP_DIR = $(BASE_DIR)/x300/ip

# Include makefiles and sources for all IP components
# *after* defining the IP_DIR
include $(IP_DIR)/fifo_short_2clk/Makefile.inc
include $(IP_DIR)/fifo_4k_2clk/Makefile.inc

DESIGN_SRCS += $(abspath \
$(IP_FIFO_4K_2CLK_SRCS) \
$(IP_FIFO_SHORT_2CLK_SRCS) \
)

# DESIGN_SRCS and VERILOG_DEFS must be defined
.DEFAULT_GOAL := netlist
netlist: .prereqs $(DESIGN_SRCS)
	$(call BUILD_VIVADO_DESIGN,$(abspath ./build.tcl),$(TOP_MODULE),$(PART_ID))

clean:
	@rm -rf build
	@rm -rf build-ip

.PHONY: netlist clean
#
# Copyright 2012-2013 Ettus Research LLC
#

##################################################
# Project Setup
##################################################
TOP_MODULE := b205
BUILD_DIR := build-$(NAME)/
export PROJ_FILE := $(BUILD_DIR)$(TOP_MODULE).ise

# set me in a custom makefile
CUSTOM_SRCS =
CUSTOM_DEFS =

include ../Makefile.common
include ../../lib/gpif2/Makefile.srcs
include ../../lib/fifo/Makefile.srcs
include ../../lib/fifo_200/Makefile.srcs
include ../../lib/dsp/Makefile.srcs
include ../../lib/control/Makefile.srcs
include ../../lib/control_200/Makefile.srcs
include ../../lib/packet_proc/Makefile.srcs
include ../../lib/packet_proc_200/Makefile.srcs
include ../../lib/timing/Makefile.srcs
include ../../lib/vita_200/Makefile.srcs
include ../../lib/wishbone/Makefile.srcs
include ../../lib/axi/Makefile.srcs
include ../../lib/radio_200/Makefile.srcs
include coregen_dsp/Makefile.srcs


B205_COREGEN_SRCS = \
coregen/b205_clk_gen.v \
coregen/fifo_4k_2clk.xco \
coregen/fifo_short_2clk.xco \
coregen/chipscope_icon.xco \
coregen/chipscope_ila.xco

##################################################
# Project Properties
##################################################
export PROJECT_PROPERTIES := \
family "Spartan6" \
device $(DEVICE) \
package csg484 \
speed -3 \
top_level_module_type "HDL" \
synthesis_tool "XST (VHDL/Verilog)" \
simulator "ISE Simulator (VHDL/Verilog)" \
"Preferred Language" "Verilog" \
"Enable Message Filtering" FALSE \
"Display Incremental Messages" FALSE

##################################################
# Sources
##################################################
TOP_SRCS = \
b205.v \
b205_ref_pll.v \
b205_core.v \
b205_io.v \
b205.ucf \
timing.ucf \
../b200/S6CLK2PIN.v \
$(B205_COREGEN_SRCS)

SOURCES = $(abspath $(TOP_SRCS)) $(FIFO_SRCS) $(FIFO_200_SRCS) \
$(CONTROL_LIB_SRCS) $(CONTROL_LIB_200_SRCS) $(SDR_LIB_SRCS) \
$(TIMING_SRCS) $(OPENCORES_SRCS) \
$(VRT_SRCS) \
$(GPIF2_SRCS) $(PACKET_PROC_SRCS) $(PACKET_PROC_200_SRCS) \
$(WISHBONE_SRCS) \
$(TIMING_SRCS) \
$(DSP_SRCS) \
$(VITA_200_SRCS) \
$(AXI_SRCS) \
$(RADIO_200_SRCS) \
$(CAT_CAP_GEN_SRCS) \
$(COREGEN_DSP_SRCS)

##################################################
# Process Properties
##################################################
SYNTHESIZE_PROPERTIES = \
"Pack I/O Registers into IOBs" Yes \
"Optimization Effort" High \
"Optimize Instantiated Primitives" TRUE \
"Register Balancing" Yes \
"Use Clock Enable" Auto \
"Use Synchronous Reset" Auto \
"Use Synchronous Set" Auto \
"Verilog Macros" "SPARTAN6=1 $(EXTRA_DEFS) $(CUSTOM_DEFS)" \
"Other XST Command Line Options" "-loop_iteration_limit 16385"

#"Number of Clock Buffers" 8 \

TRANSLATE_PROPERTIES = \
"Macro Search Path" "$(shell pwd)/../../coregen/"

MAP_PROPERTIES = \
"Generate Detailed MAP Report" TRUE \
"Allow Logic Optimization Across Hierarchy" TRUE \
"Map to Input Functions" 4 \
"Optimization Strategy (Cover Mode)" Speed \
"Pack I/O Registers/Latches into IOBs" "For Inputs and Outputs" \
"Perform Timing-Driven Packing and Placement" TRUE \
"Map Effort Level" High \
"Extra Effort" Normal \
"Combinatorial Logic Optimization" TRUE \
"Register Duplication" TRUE

PLACE_ROUTE_PROPERTIES = \
"Place & Route Effort Level (Overall)" High

STATIC_TIMING_PROPERTIES = \
"Number of Paths in Error/Verbose Report" 10 \
"Report Type" "Error Report"

GEN_PROG_FILE_PROPERTIES = \
"Create Binary Configuration File" TRUE \
"Done (Output Events)" 5 \
"Enable Bitstream Compression" TRUE \
"Unused IOB Pins" "Pull Up"

#"Configuration Rate" 6 \
#"Enable Outputs (Output Events)" 6 \

SIM_MODEL_PROPERTIES = ""

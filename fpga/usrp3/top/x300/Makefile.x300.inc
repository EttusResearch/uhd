#
# Copyright 2008-2012 Ettus Research LLC
#

##################################################
# Project Setup
##################################################
TOP_MODULE = x300
BUILD_DIR = build-$(NAME)

# set me in a custom makefile
CUSTOM_SRCS =
CUSTOM_DEFS =

##################################################
# Include other makefiles
##################################################

include ../Makefile.common
include coregen/Makefile.srcs
include coregen_dsp/Makefile.srcs
include ../../lib/control/Makefile.srcs
include ../../lib/fifo/Makefile.srcs
include ../../lib/simple_gemac/Makefile.srcs
include ../../lib/timing/Makefile.srcs
include ../../lib/zpu/Makefile.srcs
include ../../lib/wishbone/Makefile.srcs
include ../../lib/packet_proc/Makefile.srcs
include ../../lib/vita/Makefile.srcs
include ../../lib/xge/Makefile.srcs
include ../../lib/xge_interface/Makefile.srcs
include ../../lib/dsp/Makefile.srcs
include ../../lib/io_port2/Makefile.srcs
include ../../lib/axi/Makefile.srcs

promgen: $(BIT_FILE)
	promgen -w -p bin -o $(BIN_FILE) -u 0 $<

##################################################
# Project Properties
##################################################
export PROJECT_PROPERTIES := \
family "Kintex7" \
device $(DEVICE) \
package ffg900 \
speed -2 \
top_level_module_type "HDL" \
synthesis_tool "XST (VHDL/Verilog)" \
simulator "ISim (VHDL/Verilog)" \
"Preferred Language" "Verilog" \
"Enable Message Filtering" FALSE \
"Display Incremental Messages" FALSE

##################################################
# Sources
##################################################
TOP_SRCS = \
x300.v \
x300_pcie_int.v \
x300_core.v \
x300_zpu_config.vhd \
soft_ctrl.v \
dbuf_bootram.v \
capture_ddrlvds.v \
gen_ddrlvds.v \
radio.v \
bus_int.v \
gige_phy.v \
gige_phy_mdio.v \
x300.ucf \
stc3.ucf \
timing.ucf

ifeq ($(FLOORPLAN), 1)
  ifeq ($(DEVICE), XC7K325T)
    TOP_SRCS+=floorplan_X300.ucf
  endif

  ifeq ($(DEVICE), XC7K410T)
    TOP_SRCS+=floorplan_X310.ucf
  endif
endif

ifdef BUILD_10G
ETH_SRCS+=$(TEN_GIG_SRCS) $(XGE_SRCS) $(XGE_INTERFACE_SRCS) $(abspath x300_10ge.ucf)
endif

ifdef BUILD_1G
ETH_SRCS+=$(ONE_GIG_SRCS) $(abspath x300_1ge.ucf)
endif

ifdef ETH10G_PORT0
ETH_SRCS+=$(abspath x300_10ge_port0.ucf)
endif

ifdef ETH10G_PORT1
ETH_SRCS+=$(abspath x300_10ge_port1.ucf)
endif

ifndef NO_DRAM_FIFOS
DRAM_SRCS= \
$(abspath ddr3.ucf) \
$(COREGEN_DRAM_SRCS)
else
DRAM_SRCS=
endif


SOURCES = $(abspath $(TOP_SRCS)) $(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) $(SDR_LIB_SRCS) $(SERDES_SRCS) \
$(SIMPLE_GEMAC_SRCS) $(TIMING_SRCS) $(COREGEN_DSP_SRCS) \
$(VRT_SRCS) $(UDP_SRCS) $(COREGEN_SRCS) $(EXTRAM_SRCS) \
$(ZPU_CORE_SRCS) $(WISHBONE_SRCS) \
$(ETH_SRCS) $(IOPORT2_SRCS) $(COREGEN_CHIPSCOPE_SRCS) \
$(PACKET_PROC_SRCS) $(VITA_SRCS) $(DSP_SRCS) $(DRAM_SRCS) $(AXI_SRCS)


##################################################
# Process Properties
##################################################
SYNTHESIZE_PROPERTIES = \
"Number of Clock Buffers" 8 \
"Pack I/O Registers into IOBs" Yes \
"Optimization Effort" High \
"Optimize Instantiated Primitives" TRUE \
"Register Balancing" Yes \
"Use Clock Enable" Auto \
"Use Synchronous Reset" Auto \
"Use Synchronous Set" Auto \
"Verilog Macros" "$(EXTRA_DEFS) $(CUSTOM_DEFS)"

TRANSLATE_PROPERTIES = \
"Macro Search Path" "$(shell pwd)/../../coregen/"

MAP_PROPERTIES = \
"Generate Detailed MAP Report" TRUE \
"Allow Logic Optimization Across Hierarchy" TRUE \
"Pack I/O Registers/Latches into IOBs" "For Inputs and Outputs" \
"Combinatorial Logic Optimization" TRUE \
"Register Duplication" TRUE \
"Optimization Strategy (Cover Mode)" Speed \
"Map Effort Level" High \
"Extra Effort" Normal \
"Perform Timing-Driven Packing and Placement" TRUE \
"Enable Multi-Threading 2" \
"Starting Placer Cost Table (1-100)" $$(( $$RANDOM % 100 + 1 ))
#"Map to Input Functions" 4 \

PLACE_ROUTE_PROPERTIES = \
"Place & Route Effort Level (Overall)" High \
"Extra Effort (Highest PAR level only)" Normal \
"Enable Multi-Threading 4"

STATIC_TIMING_PROPERTIES = \
"Number of Paths in Error/Verbose Report" 10 \
"Report Type" "Error Report"

GEN_PROG_FILE_PROPERTIES = \
"Configuration Rate" 6 \
"Create Binary Configuration File" TRUE \
"Done (Output Events)" 5 \
"Enable Bitstream Compression" FALSE \
"Enable Outputs (Output Events)" 6 \
"Wait for DCI Match (Output Events)" NoWait

SIM_MODEL_PROPERTIES = ""

#
# Copyright 2008 Ettus Research LLC
# 

##################################################
# xtclsh Shell and tcl Script Path
##################################################
#XTCLSH := /opt/Xilinx/10.1/ISE/bin/lin/xtclsh
XTCLSH := xtclsh
ISE_HELPER := ../tcl/ise_helper.tcl

##################################################
# Project Setup
##################################################
BUILD_DIR := build/
export TOP_MODULE := passthru
export PROJ_FILE := $(BUILD_DIR)$(TOP_MODULE).ise

##################################################
# Project Properties
##################################################
export PROJECT_PROPERTIES := \
family "Spartan-3A DSP" \
device xc3sd1800a \
package cs484 \
speed -4 \
top_level_module_type "HDL" \
synthesis_tool "XST (VHDL/Verilog)" \
simulator "ISE Simulator (VHDL/Verilog)" \
"Preferred Language" "Verilog" \
"Enable Message Filtering" FALSE \
"Display Incremental Messages" FALSE 

##################################################
# Sources
##################################################
export SOURCE_ROOT := ../../../
export SOURCES := \
top/u1e_passthru/passthru.ucf \
top/u1e_passthru/passthru.v 

##################################################
# Process Properties
##################################################
export SYNTHESIZE_PROPERTIES := \
"Number of Clock Buffers" 6 \
"Pack I/O Registers into IOBs" Yes \
"Optimization Effort" High \
"Optimize Instantiated Primitives" TRUE \
"Register Balancing" Yes \
"Use Clock Enable" Auto \
"Use Synchronous Reset" Auto \
"Use Synchronous Set" Auto

export TRANSLATE_PROPERTIES := \
"Macro Search Path" "$(shell pwd)/../../coregen/"

export MAP_PROPERTIES := \
"Allow Logic Optimization Across Hierarchy" TRUE \
"Map to Input Functions" 4 \
"Optimization Strategy (Cover Mode)" Speed \
"Pack I/O Registers/Latches into IOBs" "For Inputs and Outputs" \
"Perform Timing-Driven Packing and Placement" TRUE \
"Map Effort Level" High \
"Extra Effort" Normal \
"Combinatorial Logic Optimization" TRUE \
"Register Duplication" TRUE

export PLACE_ROUTE_PROPERTIES := \
"Place & Route Effort Level (Overall)" High 

export STATIC_TIMING_PROPERTIES := \
"Number of Paths in Error/Verbose Report" 10 \
"Report Type" "Error Report"

export GEN_PROG_FILE_PROPERTIES := \
"Configuration Rate" 6 \
"Create Binary Configuration File" TRUE \
"Done (Output Events)" 5 \
"Enable Bitstream Compression" TRUE \
"Enable Outputs (Output Events)" 6 \
"Unused IOB Pins" "Pull Up"

export SIM_MODEL_PROPERTIES := ""

##################################################
# Make Options
##################################################
all:
	@echo make proj, check, synth, bin, or clean

proj:
	PROCESS_RUN="" $(XTCLSH) $(ISE_HELPER)	

check:
	PROCESS_RUN="Check Syntax" $(XTCLSH) $(ISE_HELPER)	

synth:
	PROCESS_RUN="Synthesize - XST" $(XTCLSH) $(ISE_HELPER)	

bin:
	PROCESS_RUN="Generate Programming File" $(XTCLSH) $(ISE_HELPER)		

clean:
	rm -rf $(BUILD_DIR)



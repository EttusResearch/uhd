#
# Copyright 2008 Ettus Research LLC
# 
# This file is part of GNU Radio
# 
# GNU Radio is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
# 
# GNU Radio is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with GNU Radio; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street,
# Boston, MA 02110-1301, USA.
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
export TOP_MODULE := u1plus
export PROJ_FILE := $(BUILD_DIR)$(TOP_MODULE).ise

include ../Makefile.common
include ../../fifo/Makefile.srcs
include ../../control_lib/Makefile.srcs
include ../../sdr_lib/Makefile.srcs
include ../../serdes/Makefile.srcs
include ../../simple_gemac/Makefile.srcs
include ../../timing/Makefile.srcs
include ../../opencores/Makefile.srcs
include ../../vrt/Makefile.srcs
include ../../udp/Makefile.srcs
include ../../coregen/Makefile.srcs
include ../../extram/Makefile.srcs
include ../../gpif/Makefile.srcs

##################################################
# Project Properties
##################################################
export PROJECT_PROPERTIES := \
family "Spartan3A" \
device XC3S1400A \
package ft256 \
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
TOP_SRCS = \
u1plus.v \
u1plus_core.v \
u1plus.ucf \
timing.ucf

SOURCES = $(abspath $(TOP_SRCS)) $(FIFO_SRCS) \
$(CONTROL_LIB_SRCS) $(SDR_LIB_SRCS) $(SERDES_SRCS) \
$(SIMPLE_GEMAC_SRCS) $(TIMING_SRCS) $(OPENCORES_SRCS) \
$(VRT_SRCS) $(UDP_SRCS) $(COREGEN_SRCS) $(EXTRAM_SRCS) \
$(GPIF_SRCS)

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



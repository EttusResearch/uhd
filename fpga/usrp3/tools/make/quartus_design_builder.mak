#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(BASE_DIR)/../tools/make/quartus_preamble.mak
SIMULATION = 0

# -------------------------------------------------------------------
# Usage: BUILD_QUARTUS_DESIGN
# Args: $1 = PROJECT_NAME
#       $2 = ARCH (max10, etc)
#       $3 = PART_ID (10M04SAU169I7G, etc)
#       $4 = PROJECT_DIR (Absolute path to the top level project dir)
#       $5 = BUILD_DIR (Absolute path to the top level build dir)
#       $6 = (optional) POST_STA_TCL (Absolute path to the tcl script to be run by quartus_sta)
#       $7 = (optional) PARTITION_MERGE_ON (Request partition merge in the build process)
# Prereqs:
# - TOOLS_DIR must be defined globally
# - BUILD_DIR must be defined globally
# - DESIGN_SRCS must be defined and should contain all source files
# - VERILOG_DEFS must be defined and should contain all PP defines
# -------------------------------------------------------------------
# Reports parsing performed:
#   grep for unconstrained path warning (332102) in the *.sta.rpt
#   grep for timing closure critical warning (332148) in the *.sta.rpt
#   expect no warnings in the *.sta.rpt
#   expect no critical warning except "review power analyzer report file" (16562)
BUILD_QUARTUS_DESIGN = \
	@ \
	echo "========================================================"; \
	echo "BUILDER: Building $(1) for $(3)"; \
	echo "========================================================"; \
	echo "BUILDER: Staging Quartus sources in build directory..."; \
	cp -rf $(4)/quartus/* $(5)/;\
	cd $(5); \
	echo "BUILDER: Retargeting IP to part $(2)/$(3)..."; \
	quartus_sh --set DEVICE=$(3) $(1); \
	echo "BUILDER: Synthesizing design..."; \
	quartus_map $(1) $(foreach VERILOG_DEF,$(VERILOG_DEFS),--verilog_macro=$(VERILOG_DEF)); \
	if [ $(7) -eq 1 ]; then \
		echo "BUILDER: Partition merge..."; \
		quartus_cdb $(1) --merge=on --incremental_compilation_import; \
	fi; \
	echo "BUILDER: Implementating design..."; \
	quartus_fit $(1);\
	echo "BUILDER: Timing analysis..."; \
	quartus_sta $(1); \
	echo "BUILDER: Parsing reports..."; \
	grep "332102" output_files/$(1).sta.rpt; \
		if [ $$? -eq 0 ]; then exit 1; fi; \
	grep "332148" output_files/$(1).sta.rpt; \
		if [ $$? -eq 0 ]; then exit 1; fi; \
	grep -iw "warning" output_files/$(1).sta.rpt; \
		if [ $$? -eq 0 ]; then exit 1; fi; \
	grep -i "critical warning" output_files/* | grep -v 16562; \
		if [ $$? -eq 0 ]; then exit 1; fi; \
	if [ ! -z $(6) ]; then \
		echo "BUILDER: Running additional STA TCL script..."; \
		quartus_sta -t $(6);\
	fi; \
	echo "BUILDER: Generating bitfile..."; \
	quartus_asm $(1);

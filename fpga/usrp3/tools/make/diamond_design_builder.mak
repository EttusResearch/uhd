#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-2.0-or-later
#

include $(BASE_DIR)/../tools/make/diamond_preamble.mak
SIMULATION = 0

# -------------------------------------------------------------------
# Usage: BUILD_DIAMOND_DESIGN
# Args: $1 = PROJECT_NAME
#       $2 = PART_ID (LCMXO3LF-9400C, etc)
#       $3 = PROJECT_DIR (Absolute path to the top level project dir)
#       $4 = BUILD_DIR (Absolute path to the top level build dir)
#       $5 = IMPLEMENTATION_NAME (name of design implementation in project)
# Prereqs:
# - TOOLS_DIR must be defined globally
# - DESIGN_SRCS must be defined and should contain all source files
BUILD_DIAMOND_DESIGN = \
	@ \
	echo "========================================================"; \
	echo "BUILDER: Building $(1) for $(2)"; \
	echo "========================================================"; \
	echo "BUILDER: Staging Diamond sources in build directory..."; \
	cp -rf $(3)/lattice/* $(4)/;\
	echo "BUILDER: Updating environment..."; \
	export DMD_GIT_HASH=$(GIT_HASH_VERILOG_DEF); \
	export DMD_PROJECT_FILE="$(1).ldf"; \
	export DMD_IMPL=$(5); \
	export LC_ALL=en_US.UTF-8; \
	cp $(TOOLS_DIR)/scripts/dmd_design_build.tcl $(4)/build.tcl; \
	cd $(4); \
	echo "BUILDER: Checking variables..."; \
	python3 $(TOOLS_DIR)/scripts/diamond_check_variables.py $(1).lpf; \
	if [ $$? -ne 0 ]; \
		then \
			echo "BUILDER: Diamond variable check failed."; \
			exit 1; \
		fi; \
	echo "BUILDER: Implementating design..."; \
	$(DIAMOND_EXE) build.tcl > $(1)_log.txt ; \
	if [ $$? -ne 0 ]; \
	then \
		echo "BUILDER: Build failed. See $(1)_log.txt for details."; \
		exit 1; \
	fi; \
	echo "BUILDER: Parsing reports..."; \
	grep "Cumulative negative slack: 0 (0+0)" impl1/$(1)_impl1.twr > /dev/null 2>&1; \
		if [ $$? -ne 0 ]; \
		then \
			echo "BUILDER: Timing failed. See $(1)_impl1.twr for details."; \
			exit 1; \
		fi; \
	grep "unconstrained" impl1/$(1)_impl1.twr | grep -v "0 unconstrained" > /dev/null 2>&1; \
		if [ $$? -eq 0 ]; \
		then \
			echo "BUILDER: Unconstrained paths found. See $(1)_impl1.twr for details."; \
			exit 1; \
		fi; \
	grep "Semantic error" impl1/$(1)_impl1.mrp > /dev/null 2>&1; \
		if [ $$? -eq 0 ]; \
		then \
			echo "BUILDER: Semantic error found. See $(1)_impl1.mrp for details."; \
			exit 1; \
		fi; \
	echo "BUILDER: Generating bitfile..."; \
	ddtcmd -oft -svfsingle -if $(5)/$(1)_$(5).jed			 	\
		-dev $(2) -op "FLASH Erase,Program,Verify" -revd	\
		-of $(5)/$(1)_$(5).svf;

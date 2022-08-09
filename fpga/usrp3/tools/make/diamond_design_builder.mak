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
	cd $(4); \
	echo "BUILDER: Implementating design..."; \
	pnmainc build.tcl > $(1)_log.txt ; \
	echo "BUILDER: Generating bitfile..."; \
	ddtcmd -oft -svfsingle -if $(5)/$(1)_$(5).jed			 	\
		-dev $(2) -op "FLASH Erase,Program,Verify" -revd	\
		-of $(5)/$(1)_$(5).svf;

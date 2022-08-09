#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

QSYS_PATH=$(subst \,/,$(QSYS_ROOTDIR))

# -------------------------------------------------------------------
# Usage: BUILD_QUARTUS_IP
# Args: $1 = IP_NAME (IP name)
#       $2 = ARCH (max10, etc)
#       $3 = PART_ID (10M04SAU169I7G, etc)
#       $4 = IP_SRC_DIR (Absolute path to the top level ip src dir)
#       $5 = IP_BUILD_DIR (Absolute path to the top level ip build dir)
# Prereqs:
# - TOOLS_DIR must be defined globally
# -------------------------------------------------------------------
BUILD_QUARTUS_IP = \
	@ \
	echo "========================================================"; \
	echo "BUILDER: Building IP $(1)"; \
	echo "========================================================"; \
	echo "BUILDER: Staging IP in build directory..."; \
	rm -rf $(5)/$(1); \
	mkdir -p $(5)/$(1); \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) reserve; \
	cp -rf $(4)/$(1)/* $(5)/$(1); \
	echo "BUILDER: Retargeting IP to part $(2)/$(3)..."; \
	$(QSYS_PATH)/qsys-generate $(call RESOLVE_PATH,$(5)/$(1)/$(1).qsys) --part=$(3) --simulation=VERILOG; \
	$(TOOLS_DIR)/scripts/shared-ip-loc-manage.sh --path=$(5)/$(1) release; \
	echo $?

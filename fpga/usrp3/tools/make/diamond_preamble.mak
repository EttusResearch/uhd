#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# -------------------------------------------------------------------
# Environment Setup
# -------------------------------------------------------------------
ifeq ($(VIV_PLATFORM),Cygwin)
RESOLVE_PATH = $(if $(1),$(subst \,/,$(shell cygpath -aw $(1))))
RESOLVE_PATHS = "$(if $(1),$(foreach path,$(1),$(subst \,/,$(shell cygpath -aw $(abspath $(path))))))"
else
RESOLVE_PATH = $(1)
RESOLVE_PATHS = "$(1)"
endif

# -------------------------------------------------------------------
# Project Setup
# -------------------------------------------------------------------
# Requirement: BASE_DIR must be defined

TOOLS_DIR  = $(BASE_DIR)/../tools
LIB_DIR    = $(BASE_DIR)/../lib

O ?= .

ifdef NAME
BUILD_DIR = $(abspath $(O)/build-$(NAME))
else
BUILD_DIR = $(abspath $(O)/build)
endif

IP_BUILD_DIR = $(abspath ./build-ip/$(subst /,,$(PART_ID)))

# -------------------------------------------------------------------
# Git Hash Retrieval
# -------------------------------------------------------------------
GIT_HASH = $(shell $(TOOLS_DIR)/scripts/git-hash.sh)
GIT_HASH_VERILOG_DEF = "GIT_HASH=32'h$(GIT_HASH)"

# -------------------------------------------------------------------
# Toolchain dependency target
# -------------------------------------------------------------------
.check_lscc_tool:
	@echo "BUILDER: Checking tools..."
	@echo -n "* "; bash --version | grep bash || (echo "ERROR: Bash not found in environment. Please install it"; exit 1;)
	@echo -n "* "; python3 --version || (echo "ERROR: Python not found in environment. Please install it"; exit 1;)
	@echo -n "* "; which pnmainc 2>&1 | grep diamond|| (echo "ERROR: Diamond TCL Console not found in environment."; exit 1;)

# -------------------------------------------------------------------
# Intermediate build dirs
# -------------------------------------------------------------------
.build_dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(IP_BUILD_DIR)

.diamond_prereqs: .check_lscc_tool .build_dirs

.PHONY: .check_lscc_tool .build_dirs .diamond_prereqs

# -------------------------------------------------------------------
# Validate prerequisites
# -------------------------------------------------------------------
ifndef PART_ID
	$(error PART_ID was empty or not set)
endif

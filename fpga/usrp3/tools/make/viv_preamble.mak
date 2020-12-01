#
# Copyright 2014-2015 Ettus Research
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

TOOLS_DIR  = $(abspath $(BASE_DIR)/../tools)
LIB_DIR    = $(abspath $(BASE_DIR)/../lib)
SIMLIB_DIR = $(abspath $(BASE_DIR)/../sim)
LIB_IP_DIR = $(abspath $(LIB_DIR)/ip)
HLS_IP_DIR = $(abspath $(LIB_DIR)/hls)

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
GIT_HASH=$(shell $(TOOLS_DIR)/scripts/git-hash.sh --hashfile=$(TOOLS_DIR)/../../project.githash)
GIT_HASH_VERILOG_DEF=$(addprefix GIT_HASH=32'h,$(GIT_HASH))

# -------------------------------------------------------------------
# GUI Mode switch. Calling with GUI:=1 will launch Vivado GUI for build
# -------------------------------------------------------------------
ifeq ($(GUI),1)
VIVADO_MODE=gui
else
VIVADO_MODE=batch
endif

# -------------------------------------------------------------------
# Toolchain dependency target
# -------------------------------------------------------------------
.check_tool:
	@echo "BUILDER: Checking tools..."
	@echo -n "* "; bash --version | grep bash || (echo "ERROR: Bash not found in environment. Please install it"; exit 1;)
	@echo -n "* "; python3 --version || (echo "ERROR: Python not found in environment. Please install it"; exit 1;)
	@echo -n "* "; vivado -version 2>&1 | grep Vivado || (echo "ERROR: Vivado not found in environment. Please run setupenv.sh"; exit 1;)

# -------------------------------------------------------------------
# Intermediate build dirs 
# -------------------------------------------------------------------
.build_dirs:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(IP_BUILD_DIR)

.prereqs: .check_tool .build_dirs

.PHONY: .check_tool .build_dirs .prereqs

# -------------------------------------------------------------------
# Validate prerequisites
# -------------------------------------------------------------------
ifndef PART_ID
	$(error PART_ID was empty or not set)
endif

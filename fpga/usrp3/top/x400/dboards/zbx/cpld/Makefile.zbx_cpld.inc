#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

##################################################
# Project Setup
##################################################
# TOP_MODULE = <Input arg>
# NAME = <Input arg>
# PART_ID = <Input arg>
# ARCH = <Input arg>

##################################################
# Include other makefiles
##################################################

PROJECT_DIR = $(abspath .)
BASE_DIR = $(abspath ../../../../)
IP_DIR = $(abspath ./ip)
include $(BASE_DIR)/../tools/make/quartus_design_builder.mak
include $(BASE_DIR)/../tools/make/diamond_design_builder.mak

# Include IP directory
include $(IP_DIR)/Makefile.inc

# Define VERILOG_DEFS for macros definition
VERILOG_DEFS=$(EXTRA_DEFS) $(GIT_HASH_VERILOG_DEF)

# Memory initialization files (CPLD default values)
REGS_PY_FILE=$(PROJECT_DIR)/../../../../../../../host/lib/ic_reg_maps/gen_zbx_cpld_regs.py
INIT_FILES_DIR=$(PROJECT_DIR)/register_endpoints/memory_init_files/
REGS_PY_MODULE=$(INIT_FILES_DIR)/zbx_cpld_regs_t.py

$(REGS_PY_MODULE): $(REGS_PY_FILE)
	@python3 $(REGS_PY_FILE) $(REGS_PY_MODULE)

# Using one of the files as a dependency (all files are generated at the same time)
INIT_FILES := $(INIT_FILES_DIR)/rx0_path_defaults.hex

$(INIT_FILES): $(REGS_PY_MODULE) $(INIT_FILES_DIR)/gen_defaults.py
	@python3 $(INIT_FILES_DIR)/gen_defaults.py

cpld_defaults: .prereqs $(INIT_FILES)
	@echo "Initialization files DONE ..."

bin: .prereqs
	$(call BUILD_QUARTUS_DESIGN,$(TOP_MODULE),$(ARCH),$(PART_ID),$(PROJECT_DIR),$(BUILD_DIR),$(POST_TCL_SCRIPT),0)
	@\
	pushd $(BUILD_DIR); \
	echo "Converting bitfile to svf format (ISP enabled)..."; \
	quartus_cpf --convert \
		--frequency 12.5MHz \
		--voltage 2.5       \
		--operation p       \
		output_files/$(TOP_MODULE).pof output_files/$(TOP_MODULE)_isp_on.svf -o background_programming=on; \
	echo "Converting bitfile to svf format (ISP disabled)..."; \
	quartus_cpf --convert \
		--frequency 12.5MHz \
		--voltage 2.5       \
		--operation p       \
		output_files/$(TOP_MODULE).pof output_files/$(TOP_MODULE)_isp_off.svf; \
	echo "Converting bitfile to rpd format..."; \
	quartus_cpf -c raw_conversion.cof; \
	popd;

quar_ip: .prereqs ip
	@echo "IP Build DONE ..."

dmd_build : .diamond_prereqs
	$(call BUILD_DIAMOND_DESIGN,$(TOP_MODULE),$(PART_ID),$(PROJECT_DIR),$(BUILD_DIR),$(IMPL))


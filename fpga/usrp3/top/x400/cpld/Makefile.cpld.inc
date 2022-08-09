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
BASE_DIR = $(abspath ../..)
IP_DIR = $(abspath ./ip)
include $(BASE_DIR)/../tools/make/quartus_design_builder.mak

# Include IP directory
include $(IP_DIR)/Makefile.inc

# Define VERILOG_DEFS for macros definition
VERILOG_DEFS=$(EXTRA_DEFS) $(GIT_HASH_VERILOG_DEF)

bin: .prereqs
	$(call BUILD_QUARTUS_DESIGN,$(TOP_MODULE),$(ARCH),$(PART_ID),$(PROJECT_DIR),$(BUILD_DIR),$(POST_TCL_SCRIPT),1)
	@\
	pushd $(BUILD_DIR); \
	echo "Converting bitfile to svf format (ISP enabled)..."; \
	quartus_cpf --convert \
		--frequency 10.0MHz \
		--voltage 2.5       \
		--operation p       \
		output_files/$(TOP_MODULE).pof output_files/$(TOP_MODULE)_isp_on.svf -o background_programming=on; \
	echo "Converting bitfile to svf format (ISP disabled)..."; \
	quartus_cpf --convert \
		--frequency 10.0MHz \
		--voltage 2.5       \
		--operation p       \
		output_files/$(TOP_MODULE).pof output_files/$(TOP_MODULE)_isp_off.svf; \
	echo "Converting bitfile to rpd format..."; \
	quartus_cpf -c raw_conversion.cof; \
	popd;

quar_ip: .prereqs ip
	@echo "IP Build DONE ..."

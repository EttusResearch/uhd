#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# NOTE: All comments prefixed with a "##" will be displayed as a part of the "make help" target
##-------------------
##USRP OBX CPLD Help
##-------------------
##Usage:
## make <Targets> <Options>
##
##Output:
## build/<device-id>/usrp_obx_cpld_10m04.pof:            Bitstream to use with JTAG programmer
## build/<device-id>/usrp_obx_cpld_10m04.svf:            Bitstream to use with PS JTAG engine (background programming)
## build/<device-id>/usrp_obx_cpld_10m04.rpd:            Bitstream to use via reconfig engine
## build/<device-id>/usrp_obx_cpld_10m04_isp_off.svf:    Bitstream to use with JTAG test points (initial programming)

# Definitions
10M04_ID = "10M04SAU169I7G"

# Target specific variables
OBX_CPLD_10M04: DEFS = VARIANT_`echo $(10M04_ID) | cut -c1-5`=1

TARGET = bin
TOP ?= obx_cpld

# quartus_build($1=Device, $2=Definitions)
quartus_build = make -f Makefile.obx_cpld.inc $(TARGET) NAME=$@ ARCH="MAX10" PART_ID="$1" $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2" POST_STA_TCL="ps_cs_analysis.tcl"

# post_quar_build($1=Artifact Name)
ifeq ($(TARGET),bin)
	post_quar_build = @\
		mkdir -p build/; \
		echo "Exporting bitstream files..."; \
		cp build-$@/output_files/$(TOP).pof                      build/$(1)_10m04.pof; \
		cp build-$@/output_files/$(TOP)_isp_off.svf              build/$(1)_10m04_isp_off.svf; \
		cp build-$@/output_files/$(TOP)_isp_on.svf               build/$(1)_10m04.svf; \
		cp build-$@/output_files/$(TOP)_converted_cfm0_auto.rpd  build/$(1)_10m04.rpd; \
		echo -ne "\n\n---- Make: MB CPLD ready!\n"; \
		echo -ne "     Use build/$(1).pof via JTAG programmer or\n"; \
		echo -ne "         build/$(1).svf (ISP on) via PS JTAG-engine (background programming) or\n"; \
		echo -ne "         build/$(1).rpd via reconfig engine or\n"; \
		echo -ne "         build/$(1)_isp_off.svf via JTAG test points (initial programming)\n";
else
	post_quar_build = @echo "Skipping bitfile export."
endif

##
##Supported Targets
##-----------------

all:              OBX_CPLD_10M04  ##(Build all targets by default)

##OBX_CPLD_10M04:   OBX CPLD targeted to 10M04SAU169I7G.
OBX_CPLD_10M04: $(INIT_FILES)
	$(call quartus_build,$(10M04_ID),$(DEFS))
	$(call post_quar_build,"usrp_obx_cpld")

clean:            ##Clean up all target build outputs.
	@echo -ne "\nCleaning targets and git repo...\n";
	@rm -rf build-OBX_CPLD*
	@rm -rf build
	@git clean -Xdf

help:             ##Show this help message.
	@grep -h "##" Makefile | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

##
##Supported Options
##-----------------
##

.PHONY: all build clean ip

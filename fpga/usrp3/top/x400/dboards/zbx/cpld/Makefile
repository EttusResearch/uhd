#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# NOTE: All comments prefixed with a "##" will be displayed as a part of the "make help" target
##-------------------
##USRP ZBX CPLD Help
##-------------------
##Usage:
## make <Targets> <Options>
##
##Output:
## build/<device-id>/usrp_zbx_cpld.pof:            Bitstream to use with JTAG programmer
## build/<device-id>/usrp_zbx_cpld.svf:            Bitstream to use with PS JTAG engine (background programming)
## build/<device-id>/usrp_zbx_cpld.rpd:            Bitstream to use via reconfig engine
## build/<device-id>/usrp_zbx_cpld_isp_off.svf:    Bitstream to use with JTAG test points (initial programming)

# Definitions
10M04_ID = "10M04SAU324I7G"

# Target specific variables
ZBX_CPLD_10M04: DEFS = VARIANT_`echo $(10M04_ID) | cut -c1-5`=1

# Using one of the files as a dependency (all files are generated at the same time)
INIT_FILES := register_endpoints/memory_init_files/rx0_path_defaults.hex

TARGET = bin
TOP ?= zbx_top_cpld

# pre_build()
pre_build = @\
	mkdir -p build-$@/register_endpoints/memory_init_files/; \
	cp -rf register_endpoints/memory_init_files/*.hex build-$@/register_endpoints/memory_init_files/

# quartus_build($1=Device, $2=Definitions)
quartus_build = make -f Makefile.zbx_cpld.inc $(TARGET) NAME=$@ ARCH="MAX10" PART_ID="$1" $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2" POST_STA_TCL="ps_cs_analysis.tcl"

# quartus_ip($1=Device, $2=Definitions)
quartus_ip = make -f Makefile.zbx_cpld.inc quar_ip NAME=$@ ARCH="MAX10" PART_ID="$1" $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2"

# post_build($1=Artifact Name)
ifeq ($(TARGET),bin)
	post_build = @\
		mkdir -p build/; \
		echo "Exporting bitstream files..."; \
		cp build-$@/output_files/$(TOP).pof                      build/$(1).pof; \
		cp build-$@/output_files/$(TOP)_isp_off.svf              build/$(1)_isp_off.svf; \
		cp build-$@/output_files/$(TOP)_isp_on.svf               build/$(1).svf; \
		cp build-$@/output_files/$(TOP)_converted_cfm0_auto.rpd  build/$(1).rpd; \
		echo -ne "\n\n---- Make: MB CPLD ready!\n"; \
		echo -ne "     Use build/$(1).pof via JTAG programmer or\n"; \
		echo -ne "         build/$(1).svf (ISP on) via PS JTAG-engine (background programming) or\n"; \
		echo -ne "         build/$(1).rpd via reconfig engine or\n"; \
		echo -ne "         build/$(1)_isp_off.svf via JTAG test points (initial programming)\n";
else
	post_build = @echo "Skipping bitfile export."
endif

##
##Supported Targets
##-----------------

all:              ZBX_CPLD_10M04 ##(Default target)

##ZBX_CPLD_10M04:   ZBX CPLD targeted to 10M04SAU169I7G.
ZBX_CPLD_10M04: $(INIT_FILES)
	$(call pre_build)
	$(call quartus_build,$(10M04_ID),$(DEFS))
	$(call post_build,"usrp_zbx_cpld")

ZBX_CPLD_IP:      ##Build IPs only, needed for simulation.
	@# Building only ZBX_CPLD_10M04 IP
	$(call quartus_ip,$(10M04_ID),$(DEFS))

$(INIT_FILES):
	make -f Makefile.zbx_cpld.inc cpld_defaults

clean:            ##Clean up all target build outputs.
	@echo -ne "\nCleaning targets and git repo...\n";
	@rm -rf build-ZBX_CPLD*
	@rm -rf build
	@git clean -Xdf

cleanall:         ##Clean up all target and ip build outputs.
	@echo -ne "\nCleaning targets, IP, and git repo...\n";
	@rm -rf build-ZBX_CPLD*
	@rm -rf build
	@rm -rf build-ip
	@git clean -Xdf

help:             ##Show this help message.
	@grep -h "##" Makefile | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

##
##Supported Options
##-----------------
##

.PHONY: all build clean ip

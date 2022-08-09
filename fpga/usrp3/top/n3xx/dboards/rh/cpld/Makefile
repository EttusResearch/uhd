#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# NOTE: All comments prefixed with a "##" will be displayed as a part of the "make help" target
##-------------------
##USRP N32x CPLD Help
##-------------------
##Usage:
## make <Targets> <Options>
##
##Output:
## build/usrp_n320_rh_cpld.pof:  Bitstream to use with JTAG programmer
## build/usrp_n320_rh_cpld.svf:  Bitstream to use with PS JTAG engine

# Definitions
10M04_ID = "10M04SAU169I7G"

TARGET = bin
TOP ?= rhodium_top

# quartus_build($1=Device, $2=Definitions)
quartus_build = make -f Makefile.cpld.inc $(TARGET) NAME=$@ ARCH="MAX10" PART_ID="$1" $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2"

# post_build($1=Artifact Name)
ifeq ($(TARGET),bin)
	post_build = @\
		mkdir -p build/; \
		echo "Exporting bitstream files..."; \
		cp build-$@/output_files/$(TOP).pof                      build/$(1).pof; \
		cp build-$@/output_files/$(TOP).svf                      build/$(1).svf; \
		echo -ne "\n\n---- Make: CPLD ready!\n"; \
		echo -ne "     Use build/$(1).pof via JTAG programmer or\n"; \
		echo -ne "         build/$(1).svf via PS JTAG-engine\n";
else
	post_build = @echo "Skipping bitfile export."
endif

##
##Supported Targets
##-----------------

all:              N320_CPLD_10M04 ##(Default target)

##N320_CPLD_10M04:  N320/1 CPLD targeted to 10M04SAU169I7G.
N320_CPLD_10M04:
	$(call quartus_build,$(10M04_ID),$(DEFS))
	$(call post_build,"usrp_n320_rh_cpld")

clean:            ##Clean up all target build outputs.
	@echo -ne "\nCleaning targets and git repo...\n";
	@rm -rf build-N320_CPLD*
	@rm -rf build
	@git clean -Xdf

cleanall:         ##Clean up all target and ip build outputs.
	@echo -ne "\nCleaning targets, IP, and git repo...\n";
	@rm -rf build-N320_CPLD*
	@rm -rf build
	@rm -rf build-ip
	@git clean -Xdf

help:             ##Show this help message.
	@grep -h "##" Makefile | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'


SIM_SRCS=rh_tb.v rhodium_top.v rhodium_gain_ctrl.v rhodium_gain_table.v rhodium_lo_gain.v

a.out: $(SIM_SRCS)
	iverilog -tvvp -D$(SHORT_HASH) -s rh_tb $(SIM_SRCS)


##
##Supported Options
##-----------------
##

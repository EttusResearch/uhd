#
# Copyright 2015 Ettus Research LLC
#

# NOTE: All comments prefixed with a "##" will be displayed as a part of the "make help" target
##-------------------
##USRP B2XXXmini FPGA Help
##-------------------
##Usage:
## make <Targets> <Options>
##
##Output:
## build/usrp_<product>_fpga.bit:    Configuration bitstream with header
## build/usrp_<product>_fpga.bin:    Configuration bitstream without header
## build/usrp_<product>_fpga.syr:    Xilinx system report
## build/usrp_<product>_fpga.twr:    Xilinx timing report

print_report = \
	echo "========================================================================"; \
	cat $(1)/b205.syr | grep "Device utilization summary:" -A 30; \
	echo "========================================================================"; \
	echo "Timing Summary:\n"; \
	cat $(1)/b205.twr | grep constraint | grep met | grep -v "*"; \
	echo "========================================================================";

# pre_build($1=Device)
ifeq ($(EXPORT_ONLY),1)
	pre_build = @test -s build-$(1)/b205.bit || { echo "EXPORT_ONLY requires the project in build-$(1) to be fully built."; false; }
else
	pre_build = @echo "ISE Version: $(shell xtclsh -h | head -n1)"
endif

# ise_build($1=Device, $2=Definitions)
ifeq ($(PROJECT_ONLY),1)
	ise_build = make -f Makefile.b205.inc proj NAME=$@ DEVICE=$1
else ifeq ($(EXPORT_ONLY),1)
	ise_build = @echo "Skipping ISE build and exporting pre-built files.";
else
	ise_build = make -f Makefile.b205.inc bin NAME=$@ DEVICE=$1
endif

# post_build($1=Device)
ifeq ($(PROJECT_ONLY),1)
	post_build = \
		@echo "Generated $(shell pwd)/build-$(1)/b205.xise"; \
		echo "\nProject Generation DONE ... $(1)\n";
else
	post_build = \
		@$(call print_report,build-$(1)) \
		mkdir -p build; \
		echo "Exporting bitstream files..."; \
		cp build-$(1)/b205.bin build/usrp_`echo $(1) | tr A-Z a-z`_fpga.bin; \
		cp build-$(1)/b205.bit build/usrp_`echo $(1) | tr A-Z a-z`_fpga.bit; \
		echo "Exporting logs..."; \
		cp build-$(1)/b205.syr build/usrp_`echo $(1) | tr A-Z a-z`_fpga.syr; \
		cp build-$(1)/b205.twr build/usrp_`echo $(1) | tr A-Z a-z`_fpga.twr; \
		echo "\nBuild DONE ... $(1)\n";
endif

##
##Supported Targets
##-----------------

all:      B200mini B205mini  ##(Default target)

B200mini: ##Build USRP B200mini design.
	$(call pre_build,B200mini)
	$(call ise_build,XC6SLX75)
	$(call post_build,B200mini)

B205mini: ##Build USRP B205mini design.
	$(call pre_build,B205mini)
	$(call ise_build,XC6SLX150)
	$(call post_build,B205mini)

clean:    ##Clean up all build output.
	rm -rf build-*
	rm -rf build

help:    ## Show this help message.
	@grep -h "##" Makefile | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

##
##Supported Options
##-----------------
##PROJECT_ONLY=1   Only create a Xilinx project for the specified target(s).
##                 Useful for use with the ISE GUI.
##EXPORT_ONLY=1    Export build targets from a GUI build to the build directory.
##                 Requires the project in build-*_* to be built.

.PHONY: all clean help B200mini B205mini

#
# Copyright 2025 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# NOTE: All comments prefixed with a "##" will be displayed as a part of the "make help" target
##-------------------
##USRP B310 FPGA Help
##-------------------
##Usage:
##  rfnoc_image_builder -y <image core file> [-t <target>]
##
## NOTE: For building B310 bitfiles, do not run make directly! Instead, use
## rfnoc_image_builder. It will create all intermediate files, set up a Vivado
## environment, and run Vivado to build a bitfile.
##
## There exist some special make targets which can be called directly like this:
##
##  make <Targets> <Options>
##
##Output:
## $(BUILD_OUTPUT_DIR)/usrp_<product>_fpga_<image_type>.bit:    Configuration bitstream with header
## $(BUILD_OUTPUT_DIR)/usrp_<product>_fpga_<image_type>.bin:    Configuration bitstream without header
## $(BUILD_OUTPUT_DIR)/usrp_<product>_fpga_<image_type>.rpt:    Build report (includes utilization and timing summary)

# Base output directory for all builds.
BUILD_BASE_DIR ?= .
# Base directory for the build outputs.
BUILD_OUTPUT_DIR ?= $(BUILD_BASE_DIR)/build

# Initialize a build seed. This can be changed to randomly affect build results.
BUILD_SEED ?= 0

BASE_DEFS= $(OPTIONS) BUILD_SEED=$(BUILD_SEED)

# Set build option (check RTL, run synthesis, or do a full build)
ifndef TARGET
	ifeq ($(CHECK), 1)
		TARGET = rtl
	else ifeq ($(SYNTH), 1)
		TARGET = synth
	else ifeq ($(IP_ONLY), 1)
		TARGET = viv_ip
	else
		TARGET = bin
	endif
endif
TOP ?= b310

# vivado_build($1=Device, $2=Definitions)
vivado_build = make -f Makefile.b310.inc $(TARGET) NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2"
vivado_ip    = make -f Makefile.b310.inc viv_ip    NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2"

# post_build($1=Device, $2=Option)
ifeq ($(TARGET),bin)
	post_build = @\
		mkdir -p $(BUILD_OUTPUT_DIR); \
		echo "Exporting bitstream files..."; \
		cp $(BUILD_DIR)/b310.bin $(BUILD_OUTPUT_DIR)/$(IMAGE_CORE_NAME).bin; \
		cp $(BUILD_DIR)/b310.bit $(BUILD_OUTPUT_DIR)/$(IMAGE_CORE_NAME).bit; \
		echo "Exporting build report..."; \
		cp $(BUILD_DIR)/build.rpt $(BUILD_OUTPUT_DIR)/$(IMAGE_CORE_NAME).rpt; \
		echo "Build DONE ... $(1)";
else
	post_build = @echo "Skipping bitfile export."
endif

##
##Supported Targets
##-----------------
##
##Reminder: Targets are built by calling
##
##   $ rfnoc_image_builder -y <image core file.yml> [-t <target>]
##
##The default <image core file.yml> is one of:
##
##- b310_rfnoc_image_core.yml
##
##Of course, a custom YAML file may be used.
##
##For all other targets, choose a suitable target image core YAML file depending
##on the device type (b310_*.yml) and provide the '--target' option
##to override the default from the YAML file.
##
##Example:
##    $ rfnoc_image_builder -y b310_rfnoc_image_core.yml -t B310
##will build the B310 target.
##

check-variables:
ifndef IMAGE_CORE_NAME
	$(error IMAGE_CORE_NAME is not set! Use rfnoc_image_builder to create valid make commands)
endif
ifndef BUILD_DIR
	$(error BUILD_DIR is not set! Use rfnoc_image_builder to create valid make commands)
endif

##B310:
B310: check-variables B310_IP
	$(call vivado_build,B310,$(BASE_DEFS) B310=1)
	$(call post_build,B310)

##
##Other Make Targets (these should be called directly)
##----------------------------------------------------

##B310_IP:  Build IP for B310 only. Use the -j option to build multiple IP blocks simultaneously.
B310_IP:
	+$(call vivado_ip,B310,$(BASE_DEFS) B310=1)


clean:    ##Clean up all target build outputs.
	@echo "Cleaning targets..."
	@rm -rf $(BUILD_BASE_DIR)/build-B3*_* $(BUILD_BASE_DIR)/build-b3* $(BUILD_BASE_DIR)/build-usrp* $(BUILD_DIR)
	@rm -rf $(BUILD_OUTPUT_DIR)

cleanall: ##Clean up all target and ip build outputs.
	@echo "Cleaning targets and IP..."
	@rm -rf build-ip
	@rm -rf $(BUILD_BASE_DIR)/build-B3*_* $(BUILD_BASE_DIR)/build-b3* $(BUILD_DIR)
	@rm -rf $(BUILD_OUTPUT_DIR)

help:     ##Show this help message.
	@grep -h "##" Makefile | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

##
##Supported Options
##-----------------
##GUI=1          Launch the build in the Vivado GUI.
##PROJECT=1      Save Vivado project file, otherwise it's created in memory.
##CHECK=1        Launch the syntax checker instead of building a bitfile.
##IP_ONLY=1      Launch the build but stop after IP generation.
##SYNTH=1        Launch the build but stop after synthesis.
##BUILD_SEED=<N> Build seed to used to affect build results. (Default is 0)
##TOP=<module>   Specify a top module for syntax checking. (Default is the bitfile top)

.PHONY: all clean cleanall help

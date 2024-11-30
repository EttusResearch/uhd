#
# Copyright 2022 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

# NOTE: All comments prefixed with a "##" will be displayed as a part of the "make help" target
##-------------------
##USRP X4xx FPGA Help
##-------------------
##Usage:
##  rfnoc_image_builder -y <image core file>
##
## NOTE: For building X4x0 bitfiles, do not run make directly! Instead, use
## rfnoc_image_builder. It will create all intermediate files, set up a Vivado
## environment, and run Vivado to build a bitfile.
##
## There exist some special make targets which can be called directly like this:
##
##  make <Targets> <Options>
##
##Output:
##  $(BUILD_OUTPUT_DIR)/usrp_<product>_fpga_<image_type>.bit:  Configuration bitstream with header
##  $(BUILD_OUTPUT_DIR)/usrp_<product>_fpga_<image_type>.dts:  Device tree source file
##  $(BUILD_OUTPUT_DIR)/usrp_<product>_fpga_<image_type>.rpt:  Build report (includes utilization and timing summary)

# Base output directory for all builds.
BUILD_BASE_DIR ?= .
# Base directory for the build outputs.
BUILD_OUTPUT_DIR ?= $(BUILD_BASE_DIR)/build

# Sane defaults for IP builds
X410_IP: DEFS += QSFP0_0=2 RF_BW=100
X440_IP: DEFS += QSFP0_0=2 RF_BW=400

# DRAM IP inclusion. Set to 1 to include DRAM memory controller in design, 0 to
# exclude it. Note that some targets exclude it regardless of this setting.
DRAM ?= 1

DEFS += $(OPTIONS)

# Initialize a build seed. This can be changed to randomly affect build results.
BUILD_SEED ?= 0
DEFS += BUILD_SEED=$(BUILD_SEED)

# Default value for incremental Vivado build is disabled.
INCR_BUILD ?= 0

# Option to stop after RTL elaboration. Use this flag as a synthesis check.
ifndef TARGET
	ifeq ($(CHECK), 1)
		TARGET = rtl
	else ifeq ($(SYNTH), 1)
		TARGET = synth
	else ifeq ($(IP_ONLY), 1)
		TARGET = viv_ip
	else ifeq ($(SECURE_CORE), 1)
		TARGET = secure_core
		TOP = secure_image_core
	else
		TARGET = bin
	endif
endif
TOP ?= x4xx

# vivado_build($1=Device (X410 or X440), $2=Definitions)
vivado_build = make -f Makefile.x4xx.inc $(TARGET) NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) EXTRA_DEFS="$2" TOP_MODULE=$(TOP) $2 INCR_BUILD=$(INCR_BUILD)
vivado_ip    = make -f Makefile.x4xx.inc viv_ip    NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) EXTRA_DEFS="$2" TOP_MODULE=$(TOP) $2

# vivado_build($1=Device)
ifeq ($(TARGET),bin)
	post_build = @\
		mkdir -p $(BUILD_OUTPUT_DIR); \
		echo "Exporting bitstream file..."; \
		cp $(BUILD_DIR)/x4xx.bit $(BUILD_OUTPUT_DIR)/$(IMAGE_CORE_NAME).bit; \
		echo "Exporting build report..."; \
		cp $(BUILD_DIR)/build.rpt $(BUILD_OUTPUT_DIR)/$(IMAGE_CORE_NAME).rpt; \
		echo "Exporting devicetree file..."; \
		cp $(BUILD_DIR)/$(1).dts $(BUILD_OUTPUT_DIR)/$(IMAGE_CORE_NAME).dts; \
		echo "Build DONE ... $(IMAGE_CORE_NAME)";
else
	post_build = @echo "Skipping bitfile export."
endif

##
##Available Targets:
##
##Reminder: Targets are built by calling
##
##   $ rfnoc_image_builder -y <image core file.yml>
##
##The following default image core files are provided:
##
##-----------------------------------|-----------|----|-----------------|-----------------|------------
##Image core name                    | Bandwidth | Ch | QSFP0           | QSFP1           | DRAM
##-----------------------------------|-----------|----|-----------------|-----------------|------------
##x410_CG_400_rfnoc_image_core.yml   | 400 MHz   | 4  | 100 GbE         | 100 GbE         | Unused
##x410_UC_200_rfnoc_image_core.yml   | 200 MHz   | 4  | Unused          | 100 GbE         | 64b x 4 Ch
##x410_X4_200_rfnoc_image_core.yml   | 200 MHz   | 4  | 4 x 10 GbE      | Unused          | 64b x 4 Ch
##x410_XG_200_rfnoc_image_core.yml   | 200 MHz   | 4  | 10 GbE (Lane 0) | 10 GbE (Lane 0) | 64b x 4 Ch
##x440_CG_400_rfnoc_image_core.yml   | 400 MHz   | 8  | 100 GbE         | 100 GbE         | Unused
##x440_CG_1600_rfnoc_image_core.yml  | 1.6 GHz   | 2  | 100 GbE         | 100 GbE         | Unused
##x440_X4_200_rfnoc_image_core.yml   | 200 MHz   | 8  | 4 x 10 GbE      | Unused          | Unused
##x440_X4_400_rfnoc_image_core.yml   | 400 MHz   | 8  | 4 x 10 GbE      | Unused          | 128b x 8 Ch
##x440_X4_1600_rfnoc_image_core.yml  | 1.6 GHz   | 2  | 4 x 10 GbE      | Unused          | 512b x 2 Ch
##
##Of course, a custom YAML file may be used.
##
check-variables:
ifndef IMAGE_CORE_NAME
	$(error IMAGE_CORE_NAME is not set! Use rfnoc_image_builder to create valid make commands)
endif
ifndef BUILD_DIR
	$(error BUILD_DIR is not set! Use rfnoc_image_builder to create valid make commands)
endif

X410: check-variables X410_IP $(BUILD_DIR)/X410.dts
	$(call vivado_build,X410,$(DEFS) X410=1)
	$(call post_build,X410)
X440: check-variables X440_IP $(BUILD_DIR)/X440.dts
	$(call vivado_build,X440,$(DEFS) X440=1)
	$(call post_build,X440)


##
##Other Make Targets (these should be called directly)
##----------------------------------------------------

.DEFAULT_GOAL := all

X410_IP:      ##Build X410 IP only. Use the -j option to build multiple IP blocks simultaneously.
	+$(call vivado_ip,X410,$(DEFS) X410=1)

X440_IP:      ##Build X440 IP only. Use the -j option to build multiple IP blocks simultaneously.
	+$(call vivado_ip,X440,$(DEFS) X440=1)

$(BUILD_DIR)/x410-version-info.dtsi: regmap/x410/versioning_regs_regmap_utils.vh
	tools/parse_versions_for_dts.py \
		--input $< --output $@ \
		--components fpga,cpld_ifc,db_gpio_ifc,rf_core_100m,rf_core_400m

$(BUILD_DIR)/x440-version-info.dtsi: regmap/x440/versioning_regs_regmap_utils.vh
	tools/parse_versions_for_dts.py \
		--input $< --output $@ \
		--components fpga,cpld_ifc,db_gpio_ifc,rf_core_full

# The device tree source file $(BUILD_DIR)/device_tree.dts is generated by the
# RFNoC image builder.
$(BUILD_DIR)/X410.dts: $(BUILD_DIR)/device_tree.dts $(BUILD_DIR)/x410-version-info.dtsi dts/*.dtsi
	${CC} -o $@ -C -E -I dts -nostdinc -undef -x assembler-with-cpp -D__DTS__ $<

# The device tree source file $(BUILD_DIR)/device_tree.dts is generated by the
# RFNoC image builder.
$(BUILD_DIR)/X440.dts: $(BUILD_DIR)/device_tree.dts $(BUILD_DIR)/x440-version-info.dtsi dts/*.dtsi
	${CC} -o $@ -C -E -I dts -nostdinc -undef -x assembler-with-cpp -D__DTS__ $<

clean:        ##Clean up all target build outputs.
	@echo "Cleaning targets..."
	@rm -rf $(BUILD_BASE_DIR)/build-X* $(BUILD_BASE_DIR)/build-x* $(BUILD_BASE_DIR)/build-usrp* $(BUILD_DIR)
	@rm -rf $(BUILD_OUTPUT_DIR)

cleanall:     ##Clean up all target and IP build outputs.
	@echo "Cleaning targets and IP..."
	@rm -rf build-ip
	@rm -rf $(BUILD_BASE_DIR)/build-* $(BUILD_DIR)
	@rm -rf $(BUILD_OUTPUT_DIR)

help:         ##Show this help message.
	@grep -h "##" Makefile | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

##
##Supported Options
##-----------------
##INCR_BUILD=0   Use incremental Vivado build to speed up consecutive runs
##GUI=1          Launch the build in the Vivado GUI.
##PROJECT=1      Save Vivado project file, otherwise it's created in memory.
##CHECK=1        Launch the syntax checker instead of building a bitfile.
##IP_ONLY=1      Launch the build but stop after IP generation.
##SYNTH=1        Launch the build but stop after synthesis.
##SECURE_CORE=1  Launch a build of the secure image core only.
##BUILD_SEED=<N> Build seed to used to affect build results. (Default is 0)
##TOP=<module>   Specify a top module for syntax checking. (Default is the bitfile top)

.PHONY: all clean cleanall help

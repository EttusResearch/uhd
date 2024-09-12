#
# Copyright 2018-2019 Ettus Research, a National Instruments Brand
#

# NOTE: All comments prefixed with a "##" will be displayed as a part of the "make help" target
##-------------------
##USRP E32X FPGA Help
##-------------------
##Usage:
##  rfnoc_image_builder -y <image core file> -t <target>
##
## NOTE: For building E3x0 bitfiles, do not run make directly! Instead, use
## rfnoc_image_builder. It will create all intermediate files, set up a Vivado
## environment, and run Vivado to build a bitfile.
##
## There exist some special make targets which can be called directly like this:
##
##  make <Targets> <Options>
##
##Output:
## $(BUILD_OUTPUT_DIR)/usrp_<product>_fpga_<image_type>.bit:    Configuration bitstream with header
## $(BUILD_OUTPUT_DIR)/usrp_<product>_fpga_<image_type>.dts:    Device tree source file
## $(BUILD_OUTPUT_DIR)/usrp_<product>_fpga_<image_type>.rpt:    Build report (includes utilization and timing summary)

# Base output directory for all builds.
BUILD_BASE_DIR ?= .
# Base directory for the build outputs.
BUILD_OUTPUT_DIR ?= $(BUILD_BASE_DIR)/build

# Initialize a build seed. This can be changed to randomly affect build results.
BUILD_SEED ?= 0

1G_DEFS=SFP_1GBE=1   BUILD_1G=1     $(OPTIONS) BUILD_SEED=$(BUILD_SEED)
XG_DEFS=SFP_10GBE=1  BUILD_10G=1    $(OPTIONS) BUILD_SEED=$(BUILD_SEED)
AA_DEFS=SFP_AURORA=1 BUILD_AURORA=1 $(OPTIONS) BUILD_SEED=$(BUILD_SEED)

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
TOP ?= e320

# vivado_build($1=Device, $2=Definitions)
vivado_build = make -f Makefile.e320.inc $(TARGET) NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2"
vivado_ip    = make -f Makefile.e320.inc viv_ip    NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2"

# post_build($1=Target)
ifeq ($(TARGET),bin)
	post_build = @\
		mkdir -p $(BUILD_OUTPUT_DIR); \
		echo "Exporting bitstream file..."; \
		cp $(BUILD_DIR)/e320.bit $(BUILD_OUTPUT_DIR)/$(IMAGE_CORE_NAME).bit; \
		echo "Exporting build report..."; \
		cp $(BUILD_DIR)/build.rpt $(BUILD_OUTPUT_DIR)/$(IMAGE_CORE_NAME).rpt; \
		echo "Exporting devicetree file..."; \
		cp $(BUILD_DIR)/$(1).dts $(BUILD_OUTPUT_DIR)/$(IMAGE_CORE_NAME).dts; \
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
##   $ rfnoc_image_builder -y <image core file.yml> -t <target>
##
##The default image core file is called e320_rfnoc_image_core.yml.
##Of course, a custom YAML file may be used.
##
##Example:
##    $ rfnoc_image_builder -y e320_rfnoc_image_core.yml --target E320_XG
##will build the E320_XG target (10-Gigabit Ethernet on the SFP+ port).
##
##The following targets are supported:

check-variables:
ifndef IMAGE_CORE_NAME
	$(error IMAGE_CORE_NAME is not set! Use rfnoc_image_builder to create valid make commands)
endif
ifndef BUILD_DIR
	$(error BUILD_DIR is not set! Use rfnoc_image_builder to create valid make commands)
endif

##E320_1G:  1GigE on SFP+ Port.
E320_1G: check-variables E320_IP $(BUILD_DIR)/E320_1G.dts
	$(call vivado_build,E320,$(1G_DEFS) E320=1)
	$(call post_build,E320_1G)

##E320_XG:  10GigE on SFP+ Port.
E320_XG: check-variables E320_IP $(BUILD_DIR)/E320_XG.dts
	$(call vivado_build,E320,$(XG_DEFS) E320=1)
	$(call post_build,E320_XG)

##E320_AA:  Aurora on SFP+ Port.
E320_AA: check-variables E320_IP $(BUILD_DIR)/E320_AA.dts
	$(call vivado_build,E320,$(AA_DEFS) E320=1)
	$(call post_build,E320_AA)

$(BUILD_DIR)/%.dts: dts/%.dts dts/*.dtsi
	${CC} -o $@ -E -I dts -nostdinc -undef -x assembler-with-cpp -D__DTS__ $<

##
##Other Make Targets (these should be called directly)
##----------------------------------------------------
##E320_IP:  Build IP for E320 only. Use the -j option to build multiple IP blocks simultaneously.
E320_IP:
	+$(call vivado_ip,E320,$(XG_DEFS) E320=1)

clean:    ##Clean up all target build outputs.
	@echo "Cleaning targets..."
	@rm -rf $(BUILD_BASE_DIR)/build-E3*_* $(BUILD_BASE_DIR)/build-e3* $(BUILD_BASE_DIR)/build-usrp* $(BUILD_DIR)
	@rm -rf $(BUILD_OUTPUT_DIR)

cleanall: ##Clean up all target and ip build outputs.
	@echo "Cleaning targets and IP..."
	@rm -rf build-ip
	@rm -rf $(BUILD_BASE_DIR)/build-* $(BUILD_DIR)
	@rm -rf $(BUILD_OUTPUT_DIR) $(BUILD_DIR)

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

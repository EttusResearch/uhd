#
# Copyright 2018 Ettus Research LLC
#

# NOTE: All comments prefixed with a "##" will be displayed as a part of the "make help" target
##-------------------
##USRP E3XX FPGA Help
##-------------------
##Usage:
## make <Targets> <Options>
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
DEFS += BUILD_SEED=$(BUILD_SEED)

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
TOP ?= e31x

DEFAULT_IMAGE_CORE_FILE_E31X=e310_rfnoc_image_core.v
DEFAULT_EDGE_FILE_E31X=$(abspath e310_static_router.hex)

# vivado_build($1=Device, $2=Definitions)
vivado_build = make -f Makefile.e31x.inc $(TARGET) NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2" DEFAULT_RFNOC_IMAGE_CORE_FILE=$(DEFAULT_IMAGE_CORE_FILE_E31X) DEFAULT_EDGE_FILE=$(DEFAULT_EDGE_FILE_E31X)
vivado_ip    = make -f Makefile.e31x.inc viv_ip    NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2" DEFAULT_RFNOC_IMAGE_CORE_FILE=$(DEFAULT_IMAGE_CORE_FILE_E31X) DEFAULT_EDGE_FILE=$(DEFAULT_EDGE_FILE_E31X)

# post_build($1=Device, $2=Option)
ifeq ($(TARGET),bin)
	post_build = @\
		mkdir -p $(BUILD_OUTPUT_DIR); \
		echo "Exporting bitstream file..."; \
		cp $(BUILD_BASE_DIR)/build-$(1)/e31x.bit $(BUILD_OUTPUT_DIR)/usrp_`echo $(2) | tr A-Z a-z`_fpga.bit; \
		echo "Exporting build report..."; \
		cp $(BUILD_BASE_DIR)/build-$(1)/build.rpt $(BUILD_OUTPUT_DIR)/usrp_`echo $(2) | tr A-Z a-z`_fpga.rpt; \
		echo "Build DONE ... $(1)";
else
	post_build = @echo "Skipping bitfile export."
endif

##
##Supported Targets
##-----------------

all:            E310_SG1_IDLE E310_SG3_IDLE E310_SG1 E310_SG3 ##(Default target)

##E310_SG1_IP:    Build IP for E310_SG1 only.
E310_SG1_IP:
	+$(call vivado_ip,E310_SG1,E310_SG1=1)

##E310_SG3_IP:    Build IP for E310_SG3 only.
E310_SG3_IP:
	+$(call vivado_ip,E310_SG3,E310_SG3=1)

##E310_SG1_IDLE:  Build USRP E3XX idle design (Speed Grade 1).
E310_SG1_IDLE E3XX_idle: E310_SG1_IP $(BUILD_OUTPUT_DIR)/usrp_e310_sg1_idle_fpga.dts
	$(call vivado_build,E310_SG1,$(DEFS) E310_IDLE_IMAGE=1 E310_SG1=1)
	$(call post_build,$@,E310_SG1_IDLE)

##E310_SG3_IDLE:  Build USRP E3XX idle design (Speed Grade 3).
E310_SG3_IDLE E3XX_idle_sg3: E310_SG3_IP $(BUILD_OUTPUT_DIR)/usrp_e310_sg3_idle_fpga.dts
	$(call vivado_build,E310_SG3,$(DEFS) E310_IDLE_IMAGE=1 E310_SG3=1)
	$(call post_build,$@,E310_SG3_IDLE)

##E310_SG1:       Build USRP E3XX (Speed Grade 1).
E310_SG1 E310: E310_SG1_IP $(BUILD_OUTPUT_DIR)/usrp_e310_sg1_fpga.dts
	$(call vivado_build,E310_SG1,$(DEFS) E310_SG1=1 $(if $(DRAM),ENABLE_DRAM=1,))
	$(call post_build,$@,E310_SG1)

##E310_SG3:       Build USRP E3XX (Speed Grade 3).
E310_SG3 E310_sg3: E310_SG3_IP $(BUILD_OUTPUT_DIR)/usrp_e310_sg3_fpga.dts
	$(call vivado_build,E310_SG3,$(DEFS) E310_SG3=1 $(if $(DRAM),ENABLE_DRAM=1,))
	$(call post_build,$@,E310_SG3)

$(BUILD_OUTPUT_DIR)/%.dts: dts/%.dts dts/*.dtsi
	-mkdir -p $(BUILD_OUTPUT_DIR)
	${CC} -o $@ -E -I dts -nostdinc -undef -x assembler-with-cpp -D__DTS__ $<

clean:          ##Clean up all target build outputs.
	@echo "Cleaning targets..."
	@rm -rf $(BUILD_BASE_DIR)/build-E3*
	@rm -rf $(BUILD_OUTPUT_DIR)

cleanall:       ##Clean up all target and ip build outputs.
	@echo "Cleaning targets and IP..."
	@rm -rf build-ip
	@rm -rf $(BUILD_BASE_DIR)/build-E3*
	@rm -rf $(BUILD_OUTPUT_DIR)

help:           ##Show this help message.
	@grep -h "##" Makefile | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

##
##Supported Options
##-----------------
##DRAM=1         Include DDR3 SDRAM memory controller IP in the FPGA build.
##               Note: The RFNoC image core must also be configured to use DRAM.
##GUI=1          Launch the build in the Vivado GUI.
##PROJECT=1      Save Vivado project file, otherwise it's created in memory.
##CHECK=1        Launch the syntax checker instead of building a bitfile.
##IP_ONLY=1      Launch the build but stop after IP generation.
##SYNTH=1        Launch the build but stop after synthesis.
##BUILD_SEED=<N> Build seed to used to affect build results. (Default is 0)
##TOP=<module>   Specify a top module for syntax checking. (Default is the bitfile top)

.PHONY: all clean cleanall help

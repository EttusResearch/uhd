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
## build/usrp_<product>_fpga_<image_type>.bit:    Configuration bitstream with header
## build/usrp_<product>_fpga_<image_type>.dts:    Device tree source file
## build/usrp_<product>_fpga_<image_type>.rpt:    Build report (includes utilization and timing summary)

# Set build option (check RTL, run synthesis, or do a full build)
ifndef TARGET
	ifdef CHECK
		TARGET = rtl
	else ifdef SYNTH
		TARGET = synth
	else
		TARGET = bin
	endif
endif
TOP ?= e31x

DEFAULT_IMAGE_CORE_FILE_E31X=e31x_rfnoc_image_core.v
DEFAULT_EDGE_FILE_E31X=$(abspath e310_static_router.hex)

# vivado_build($1=Device, $2=Definitions)
vivado_build = make -f Makefile.e31x.inc $(TARGET) NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2" DEFAULT_RFNOC_IMAGE_CORE_FILE=$(DEFAULT_IMAGE_CORE_FILE_E31X) DEFAULT_EDGE_FILE=$(DEFAULT_EDGE_FILE_E31X)

# post_build($1=Device, $2=Option)
ifeq ($(TARGET),bin)
	post_build = @\
		mkdir -p build; \
		echo "Exporting bitstream file..."; \
		cp build-$(1)/e31x.bit build/usrp_`echo $(2) | tr A-Z a-z`_fpga.bit; \
		echo "Exporting build report..."; \
		cp build-$(1)/build.rpt build/usrp_`echo $(2) | tr A-Z a-z`_fpga.rpt; \
		echo "Build DONE ... $(1)";
else
	post_build = @echo "Skipping bitfile export."
endif

##
##Supported Targets
##-----------------

all:            E310_SG1_IDLE E310_SG3_IDLE E310_SG1 E310_SG3 E310_SG1_RFNOC E310_SG3_RFNOC ##(Default target)

##E310_SG1_IDLE:  Build USRP E3XX idle design (Speed Grade 1).
E310_SG1_IDLE E3XX_idle:  build/usrp_e310_sg1_idle_fpga.dts
	$(call vivado_build,E310_SG1, E310_IDLE_IMAGE=1 E310_SG1=1)
	$(call post_build,$@,E310_SG1_IDLE)

##E310_SG3_IDLE:  Build USRP E3XX idle design (Speed Grade 3).
E310_SG3_IDLE E3XX_idle_sg3:  build/usrp_e310_sg3_idle_fpga.dts
	$(call vivado_build,E310_SG3, E310_IDLE_IMAGE=1 E310_SG3=1)
	$(call post_build,$@,E310_SG3_IDLE)

##E310_SG1:       Build USRP E3XX (Speed Grade 1).
E310_SG1 E310:  build/usrp_e310_sg1_fpga.dts
	$(call vivado_build,E310_SG1, E310_SG1=1)
	$(call post_build,$@,E310_SG1)

##E310_SG3:       Build USRP E3XX (Speed Grade 3).
E310_SG3 E310_sg3:  build/usrp_e310_sg3_fpga.dts
	$(call vivado_build,E310_SG3, E310_SG3=1)
	$(call post_build,$@,E310_SG3)


build/%.dts: dts/%.dts dts/*.dtsi
	-mkdir -p build
	${CC} -o $@ -E -I dts -nostdinc -undef -x assembler-with-cpp -D__DTS__ $<

clean:          ##Clean up all target build outputs.
	@echo "Cleaning targets..."
	@rm -rf build-E3*
	@rm -rf build

cleanall:       ##Clean up all target and ip build outputs.
	@echo "Cleaning targets and IP..."
	@rm -rf build-ip
	@rm -rf build-E3*
	@rm -rf build

help:           ##Show this help message.
	@grep -h "##" Makefile | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

##
##Supported Options
##-----------------
##GUI=1        Launch the build in the Vivado GUI.
##CHECK=1      Launch the syntax checker instead of building a bitfile.
##SYNTH=1      Launch the build but stop after synthesis.
##TOP=<module> Specify a top module for syntax checking. (Optional. Default is the bitfile top)

.PHONY: all clean cleanall help

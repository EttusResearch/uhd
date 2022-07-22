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
##  make <Targets> <Options>
##
##Output:
##  build/usrp_<product>_fpga_<image_type>.bit:  Configuration bitstream with header
##  build/usrp_<product>_fpga_<image_type>.dts:  Device tree source file
##  build/usrp_<product>_fpga_<image_type>.rpt:  Build report (includes utilization and timing summary)



# Definitions
# MGT Types from x4xx_mgt_type.vh
MGT_100GbE   = 5
MGT_Aurora   = 3
MGT_10GbE    = 2
MGT_Disabled = 0

# For a 4-lane MGT like 100GBE set QSFPx_0=MGT_100GbE and all others to
# MGT_Disabled. For a 1-lane MGT like 10GbE do not set anything for unused
# lanes. This ensures something is defined only for the lanes that are used, so
# that the TX/RX signals are connected. The presence of a define causes TX/RX
# to be declared, whereas declaring TX/RX on an unused lane will cause an error
# in bitgen for unconstrained pins after it is optimized out.

QSFP0_10GBE   = QSFP0_0=$(MGT_10GbE)
QSFP0_4X10GBE = QSFP0_0=$(MGT_10GbE)  QSFP0_1=$(MGT_10GbE)    QSFP0_2=$(MGT_10GbE)    QSFP0_3=$(MGT_10GbE)
QSFP0_100GBE  = QSFP0_0=$(MGT_100GbE) QSFP0_1=$(MGT_Disabled) QSFP0_2=$(MGT_Disabled) QSFP0_3=$(MGT_Disabled)

QSFP1_10GBE   = QSFP1_0=$(MGT_10GbE)
QSFP1_4X10GBE = QSFP1_0=$(MGT_10GbE)  QSFP1_1=$(MGT_10GbE)    QSFP1_2=$(MGT_10GbE)    QSFP1_3=$(MGT_10GbE)
QSFP1_100GBE  = QSFP1_0=$(MGT_100GbE) QSFP1_1=$(MGT_Disabled) QSFP1_2=$(MGT_Disabled) QSFP1_3=$(MGT_Disabled)


# Target specific variables
X410_IP:      DEFS += $(QSFP0_10GBE)                   RFBW_100M=1
X410_X1_100:  DEFS += $(QSFP0_10GBE)                   RFBW_100M=1 DRAM_CH=4*$(DRAM) DRAM_W=64
X410_XG_100:  DEFS += $(QSFP0_10GBE)   $(QSFP1_10GBE)  RFBW_100M=1 DRAM_CH=4*$(DRAM) DRAM_W=64
X410_X4_100:  DEFS += $(QSFP0_4X10GBE)                 RFBW_100M=1 DRAM_CH=4*$(DRAM) DRAM_W=64
X410_X4C_100: DEFS += $(QSFP0_4X10GBE) $(QSFP1_100GBE) RFBW_100M=1 DRAM_CH=0
X410_C1_100:  DEFS += $(QSFP0_100GBE)                  RFBW_100M=1 DRAM_CH=4*$(DRAM) DRAM_W=64
X410_C1_200:  DEFS += $(QSFP0_100GBE)                  RFBW_200M=1 DRAM_CH=4*$(DRAM) DRAM_W=64
X410_X1_200:  DEFS += $(QSFP0_10GBE)                   RFBW_200M=1 DRAM_CH=4*$(DRAM) DRAM_W=64
X410_XG_200:  DEFS += $(QSFP0_10GBE)   $(QSFP1_10GBE)  RFBW_200M=1 DRAM_CH=4*$(DRAM) DRAM_W=64
X410_X4_200:  DEFS += $(QSFP0_4X10GBE)                 RFBW_200M=1 DRAM_CH=4*$(DRAM) DRAM_W=64
X410_X4C_200: DEFS += $(QSFP0_4X10GBE) $(QSFP1_100GBE) RFBW_200M=1 DRAM_CH=0
X410_X1_400:  DEFS += $(QSFP0_10GBE)                   RFBW_400M=1 DRAM_CH=4*$(DRAM) DRAM_W=128
X410_XG_400:  DEFS += $(QSFP0_10GBE)   $(QSFP1_10GBE)  RFBW_400M=1 DRAM_CH=4*$(DRAM) DRAM_W=128
X410_X4_400:  DEFS += $(QSFP0_4X10GBE)                 RFBW_400M=1 DRAM_CH=4*$(DRAM) DRAM_W=128
X410_C1_400:  DEFS += $(QSFP0_100GBE)                  RFBW_400M=1 DRAM_CH=0
X410_CG_400:  DEFS += $(QSFP0_100GBE)  $(QSFP1_100GBE) RFBW_400M=1 DRAM_CH=0

# DRAM IP inclusion. Set to 1 to include DRAM memory controller in design, 0 to
# exclude it. Note that some targets exclude it regardless of this setting.
DRAM ?= 1

DEFS += $(OPTIONS)

# Defaults specific to the various targets:
X410_100_DEFAULTS     := DEFAULT_RFNOC_IMAGE_CORE_FILE=x410_100_rfnoc_image_core.v     DEFAULT_EDGE_FILE=$(abspath x410_100_static_router.hex)
X410_200_DEFAULTS     := DEFAULT_RFNOC_IMAGE_CORE_FILE=x410_200_rfnoc_image_core.v     DEFAULT_EDGE_FILE=$(abspath x410_200_static_router.hex)
X410_400_DEFAULTS     := DEFAULT_RFNOC_IMAGE_CORE_FILE=x410_400_rfnoc_image_core.v     DEFAULT_EDGE_FILE=$(abspath x410_400_static_router.hex)
X410_400_128_DEFAULTS := DEFAULT_RFNOC_IMAGE_CORE_FILE=x410_400_128_rfnoc_image_core.v DEFAULT_EDGE_FILE=$(abspath x410_400_128_static_router.hex)


# Option to stop after RTL elaboration. Use this flag as a synthesis check.
ifndef TARGET
	ifdef CHECK
		TARGET = rtl
	else ifdef SYNTH
		TARGET = synth
	else
		TARGET = bin
	endif
endif
TOP ?= x4xx

# vivado_build($1=Device, $2=Definitions, $3=Defaults)
vivado_build = make -f Makefile.x4xx.inc $(TARGET) NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2" $3
vivado_ip    = make -f Makefile.x4xx.inc viv_ip    NAME=$@ ARCH=$(XIL_ARCH_$1) PART_ID=$(XIL_PART_ID_$1) $2 TOP_MODULE=$(TOP) EXTRA_DEFS="$2" $3

# vivado_build($1=Device, $2=Option)
ifeq ($(TARGET),bin)
	post_build = @\
		mkdir -p build; \
		echo "Exporting bitstream file..."; \
		cp build-$(1)_$(2)/x4xx.bit build/usrp_`echo $(1) | tr A-Z a-z`_fpga_$(2).bit; \
		echo "Exporting build report..."; \
		cp build-$(1)_$(2)/build.rpt build/usrp_`echo $(1) | tr A-Z a-z`_fpga_$(2).rpt; \
		echo "Build DONE ... $(1)_$(2)";
else
	post_build = @echo "Skipping bitfile export."
endif

##
##Available Targets
##------------|-----------|-----------------|-----------------|------------
##Target      | Bandwidth | QSFP0           | QSFP1           | DRAM
##------------|-----------|-----------------|-----------------|------------
##X410_X1_100 | 100 MHz   | 10 GbE (Lane 0) | None            | 64b x 4 Ch
##X410_XG_100 | 100 MHz   | 10 GbE (Lane 0) | 10 GbE (Lane 0) | 64b x 4 Ch
##X410_X4_100 | 100 MHz   | 4 x 10 GbE      | None            | 64b x 4 Ch
##X410_X1_200 | 200 MHz   | 10 GbE (Lane 0) | None            | 64b x 4 Ch
##X410_XG_200 | 200 MHz   | 10 GbE (Lane 0) | 10 GbE (Lane 0) | 64b x 4 Ch
##X410_X4_200 | 200 MHz   | 4 x 10 GbE      | None            | 64b x 4 Ch
##X410_X1_400 | 400 MHz   | 10 GbE (Lane 0) | None            | 128b x 4 Ch
##X410_XG_400 | 400 MHz   | 10 GbE (Lane 0) | 10 GbE (Lane 0) | 128b x 4 ch
##X410_X4_400 | 400 MHz   | 4 x 10 GbE      | None            | 128b x 4 Ch
##X410_C1_400 | 400 MHz   | 100 GbE         | None            | None
##X410_CG_400 | 400 MHz   | 100 GbE         | 100 GbE         | None
##* Note: Not all targets are shipped with UHD

X410_X1_100: X410_IP build/usrp_x410_fpga_X1_100.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_100_DEFAULTS))
	$(call post_build,X410,X1_100)

X410_XG_100: X410_IP build/usrp_x410_fpga_XG_100.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_100_DEFAULTS))
	$(call post_build,X410,XG_100)

X410_X4_100: X410_IP build/usrp_x410_fpga_X4_100.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_100_DEFAULTS))
	$(call post_build,X410,X4_100)

X410_X1_200: X410_IP build/usrp_x410_fpga_X1_200.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_200_DEFAULTS))
	$(call post_build,X410,X1_200)

X410_XG_200: X410_IP build/usrp_x410_fpga_XG_200.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_200_DEFAULTS))
	$(call post_build,X410,XG_200)

X410_X4_200: X410_IP build/usrp_x410_fpga_X4_200.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_200_DEFAULTS))
	$(call post_build,X410,X4_200)

X410_X1_400: X410_IP build/usrp_x410_fpga_X1_400.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_400_128_DEFAULTS))
	$(call post_build,X410,X1_400)

X410_XG_400: X410_IP build/usrp_x410_fpga_XG_400.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_400_128_DEFAULTS))
	$(call post_build,X410,XG_400)

X410_X4_400: X410_IP build/usrp_x410_fpga_X4_400.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_400_128_DEFAULTS))
	$(call post_build,X410,X4_400)

X410_C1_400: X410_IP build/usrp_x410_fpga_C1_400.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_400_DEFAULTS))
	$(call post_build,X410,C1_400)

X410_CG_400: X410_IP build/usrp_x410_fpga_CG_400.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_400_DEFAULTS))
	$(call post_build,X410,CG_400)

##
##Experimental Targets
##-------------|-----------|----------------|----------------|------------
##Target       | Bandwidth | QSFP0          | QSFP1          | DRAM
##-------------|-----------|----------------|----------------|------------
##X410_X4C_100 | 100       | 4 x 10 GbE     | 100GbE         | None
##X410_C1_100  | 100       | 100 GbE        | None           | 64b x 4 Ch
##X410_X4C_200 | 200       | 4 x 10 GbE     | 100GbE         | None
##X410_C1_200  | 200       | 100 GbE        | None           | 64b x 4 Ch

X410_X4C_100: X410_IP build/usrp_x410_fpga_X4C_100.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_100_DEFAULTS))
	$(call post_build,X410,X4C_100)

X410_C1_100: X410_IP build/usrp_x410_fpga_CG_100.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_100_DEFAULTS))
	$(call post_build,X410,CG_100)

X410_C1_200: X410_IP build/usrp_x410_fpga_CG_200.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_200_DEFAULTS))
	$(call post_build,X410,CG_200)

X410_X4C_200: X410_IP build/usrp_x410_fpga_X4C_200.dts
	$(call vivado_build,X410,$(DEFS) X410=1,$(X410_200_DEFAULTS))
	$(call post_build,X410,X4C_200)

##
##Other Make Targets
##------------------

.DEFAULT_GOAL := all

all:          X410_X4_200 X410_CG_400 ##(Default targets)

X410_IP:      ##Build IP only.
	+$(call vivado_ip,X410,$(DEFS) X410=1)

build/%.dts: dts/*.dts dts/*.dtsi
	-mkdir -p build
	tools/parse_versions_for_dts.py \
		--input regmap/versioning_regs_regmap_utils.vh \
		--output dts/x410-version-info.dtsi \
		--components fpga,cpld_ifc,db_gpio_ifc,rf_core_100m,rf_core_400m
	${CC} -o $@ -C -E -I dts -nostdinc -undef -x assembler-with-cpp -D__DTS__ \
		$$(python3 tools/get_dts_input.py --target $@)

clean:        ##Clean up all target build outputs.
	@echo "Cleaning targets..."
	@rm -rf build-X4*
	@rm -rf build

cleanall:     ##Clean up all target and IP build outputs.
	@echo "Cleaning targets and IP..."
	@rm -rf build-ip
	@rm -rf build-X4*
	@rm -rf build

help:         ##Show this help message.
	@grep -h "##" Makefile | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

##
##Supported Options
##-----------------
##DRAM=0       Exclude DDR4 memory controller IP from the FPGA build.
##GUI=1        Launch the build in the Vivado GUI.
##CHECK=1      Launch the syntax checker instead of building a bitfile.
##SYNTH=1      Launch the build but stop after synthesis.
##TOP=<module> Specify a top module for syntax checking. (Optional. Default is the bitfile top)

.PHONY: all clean cleanall help

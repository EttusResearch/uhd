#
# Copyright 2014-2015 Ettus Research
#

include $(BASE_DIR)/../tools/make/viv_preamble.mak
SIMULATION = 0

# -------------------------------------------------------------------
# Usage: BUILD_VIVADO_DESIGN
# Args: $1 = TCL_SCRIPT_NAME
#       $2 = TOP_MODULE
#       $3 = ARCH (zynq, kintex7, etc)
#       $4 = PART_ID (<device>/<package>/<speedgrade>[/<temperaturegrade>[/<silicon_revision>]])
# Prereqs:
# - TOOLS_DIR must be defined globally
# - BUILD_DIR must be defined globally
# - DESIGN_SRCS must be defined and should contain all source files
# - VERILOG_DEFS must be defined and should contain all PP defines
# - INCR_BUILD can be defined and runs parts of the Vivado flow in incremental mode
# -------------------------------------------------------------------
BUILD_VIVADO_DESIGN = \
	@ \
	$(if $(BUILD_DIR),,$(error "BUILD_DIR" is not defined! Use rfnoc_image_builder to create valid commands)) \
	$(if $(IMAGE_CORE_NAME),,$(error "IMAGE_CORE_NAME" is not defined! Use rfnoc_image_builder to create valid commands)) \
	export VIV_TOOLS_DIR=$(call RESOLVE_PATH,$(TOOLS_DIR)); \
	export VIV_OUTPUT_DIR=$(call RESOLVE_PATH,$(BUILD_DIR)); \
	export VIV_TOP_MODULE=$(2); \
	export VIV_PART_NAME=`python3 $(TOOLS_DIR)/scripts/viv_gen_part_id.py $(3)/$(4)`; \
	export VIV_MODE=$(VIVADO_MODE); \
	export VIV_PROJECT=$(VIVADO_PROJECT); \
	export VIV_DESIGN_SRCS=$(call RESOLVE_PATHS,$(call uniq,$(DESIGN_SRCS))); \
	export VIV_VERILOG_DEFS="$(VERILOG_DEFS) UHD_FPGA_DIR=$(BASE_DIR)/../.."; \
	export VIV_INCR_BUILD=$(INCR_BUILD); \
	export VIV_SECURE_KEY=$(call RESOLVE_PATH,$(abspath $(SECURE_KEY))); \
	cd $(BUILD_DIR); \
	$(TOOLS_DIR)/scripts/launch_vivado.py --parse-config $(MAKEFILE_DIR)/dev_config.json -mode $(VIVADO_MODE) -source $(call RESOLVE_PATH,$(1)) -log build.log -journal $(2).jou


# -------------------------------------------------------------------
# Usage: CHECK_VIVADO_DESIGN
# Args: $1 = TCL_SCRIPT_NAME
#       $2 = TOP_MODULE
#       $3 = ARCH (zynq, kintex7, etc)
#       $4 = PART_ID (<device>/<package>/<speedgrade>[/<temperaturegrade>[/<silicon_revision>]])
# Prereqs:
# - TOOLS_DIR must be defined globally
# - BUILD_DIR must be defined globally
# - DESIGN_SRCS must be defined and should contain all source files
# - VERILOG_DEFS must be defined and should contain all PP defines
# -------------------------------------------------------------------
CHECK_VIVADO_DESIGN = \
	@ \
	export VIV_TOOLS_DIR=$(call RESOLVE_PATH,$(TOOLS_DIR)); \
	export VIV_OUTPUT_DIR=$(call RESOLVE_PATH,$(BUILD_DIR)); \
	export VIV_TOP_MODULE=$(2); \
	export VIV_PART_NAME=`python3 $(TOOLS_DIR)/scripts/viv_gen_part_id.py $(3)/$(4)`; \
	export VIV_MODE=$(VIVADO_MODE); \
	export VIV_DESIGN_SRCS=$(call RESOLVE_PATHS,$(call uniq,$(DESIGN_SRCS))); \
	export VIV_VERILOG_DEFS="$(VERILOG_DEFS) UHD_FPGA_DIR=$(BASE_DIR)../../"; \
	cd $(BUILD_DIR); \
	$(TOOLS_DIR)/scripts/launch_vivado.py --parse-config $(TOOLS_DIR)/scripts/check_config.json -mode $(VIVADO_MODE) -source $(call RESOLVE_PATH,$(1)) -log build.log -journal $(2).jou


# Predeclare RFNOC_OOT_SRCS to make sure it's not recursively expanded
RFNOC_OOT_SRCS :=

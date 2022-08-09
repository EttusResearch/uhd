#
# Copyright 2016 Ettus Research
#

include $(BASE_DIR)/../tools/make/viv_preamble.mak
SIMULATION = 1
SIM_RUNTIME_US = 1000000000

# -------------------------------------------------------------------
# Setup simulation
# -------------------------------------------------------------------

# Define part using PART_ID (<device>/<package>/<speedgrade>)
# and architecture (zynq, kintex7, or artix7)
#
# Most simulations are part agnostic, but the user can override
# these if needed in the makefile in the simulation directory
#
ARCH ?= kintex7
PART_ID ?= xc7k410t/ffg900/-2

# Include makefiles and sources for the DUT and its dependencies
include $(BASE_DIR)/../lib/sim/Makefile.srcs

DESIGN_SRCS = $(abspath $(SIM_DESIGN_SRCS))

# Include interfaces and classes
include $(BASE_DIR)/../sim/general/Makefile.srcs
include $(BASE_DIR)/../sim/axi/Makefile.srcs
include $(BASE_DIR)/../sim/control/Makefile.srcs
include $(BASE_DIR)/../sim/rfnoc/Makefile.srcs
include $(BASE_DIR)/../sim/packages/Makefile.srcs

INC_SRCS = $(abspath \
$(SIM_GENERAL_SRCS) \
$(SIM_AXI_SRCS) \
$(SIM_CONTROL_SRCS) \
$(SIM_RFNOC_SRCS) \
$(SIM_PACKAGES_SRCS) \
)

# Predeclare RFNOC_OOT_SRCS to make sure it's not recursively expanded
RFNOC_OOT_SRCS :=

# Debug target
vars::
	env
	@echo "ARCH=$(ARCH)"
	@echo "PART_ID=$(PART_ID)"

all:
	$(error "all" or "<empty>" is not a valid target. Run make help for a list of supported targets.)

ipclean:
	@rm -rf $(abspath ./build-ip)

help::
	@echo "-----------------"
	@echo "Supported Targets"
	@echo "-----------------"
	@echo "ipclean:    Cleanup all IP intermediate files"
	@echo "clean:      Cleanup all simulator intermediate files"
	@echo "cleanall:   Cleanup everything!"

cleanall: ipclean clean

.PHONY: ipclean cleanall help

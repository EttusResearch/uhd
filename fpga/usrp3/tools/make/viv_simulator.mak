#
# Copyright 2014-2015 Ettus Research
#

# -------------------------------------------------------------------
# Mode switches
# -------------------------------------------------------------------

# Calling with FAST:=1 will switch to using unifast libs
ifeq ($(FAST),1)
SIM_FAST=true
else
SIM_FAST=false
endif

# -------------------------------------------------------------------
# Path variables
# -------------------------------------------------------------------

ifdef SIM_COMPLIBDIR
COMPLIBDIR = $(call RESOLVE_PATH,$(SIM_COMPLIBDIR))
endif

# Parse part name from ID
PART_NAME=$(subst /,,$(PART_ID))

# -------------------------------------------------------------------
# Usage: SETUP_AND_LAUNCH_SIMULATION
# Args: $1 = Simulator Name
# -------------------------------------------------------------------
SETUP_AND_LAUNCH_SIMULATION = \
	@ \
	export VIV_SIMULATOR=$1; \
	export VIV_DESIGN_SRCS=$(call RESOLVE_PATHS,$(DESIGN_SRCS)); \
	export VIV_SIM_SRCS=$(call RESOLVE_PATHS,$(SIM_SRCS)); \
	export VIV_INC_SRCS=$(call RESOLVE_PATHS,$(INC_SRCS)); \
	export VIV_SIM_TOP=$(SIM_TOP); \
	export VIV_SYNTH_TOP="$(SYNTH_DUT)"; \
	export VIV_PART_NAME=$(PART_NAME); \
	export VIV_SIM_RUNTIME=$(SIM_RUNTIME_US); \
	export VIV_SIM_FAST="$(SIM_FAST)"; \
	export VIV_SIM_COMPLIBDIR=$(COMPLIBDIR); \
	export VIV_SIM_USER_DO=$(MODELSIM_USER_DO); \
	export VIV_MODE=$(VIVADO_MODE); \
	export VIV_SIM_64BIT=$(MODELSIM_64BIT); \
	$(TOOLS_DIR)/scripts/launch_vivado.sh -mode $(VIVADO_MODE) -source $(call RESOLVE_PATH,$(TOOLS_DIR)/scripts/viv_sim_project.tcl) -log xsim.log -nojournal

.SECONDEXPANSION:

##xsim:       Run the simulation using the Xilinx Vivado Simulator
xsim: .check_tool $(DESIGN_SRCS) $(SIM_SRCS) $(INC_SRCS)
	$(call SETUP_AND_LAUNCH_SIMULATION,XSim)

##xclean:     Cleanup Xilinx Vivado Simulator intermediate files
xclean:
	@rm -f xsim*.log
	@rm -rf xsim_proj
	@rm -f xvhdl.log
	@rm -f xvhdl.pba
	@rm -f xvlog.log
	@rm -f xvlog.pb
	@rm -f vivado_pid*.str

##vsim:       Run the simulation using Modelsim
vsim: .check_tool $(COMPLIBDIR) $(DESIGN_SRCS) $(SIM_SRCS) $(INC_SRCS)
	$(call SETUP_AND_LAUNCH_SIMULATION,Modelsim)

##vlint:	  Run verilog compiler to lint files.
vlint: .check_tool
	@vlog $(SIM_SRCS) +incdir+$(BASE_DIR)/../sim/axi +incdir+$(BASE_DIR)/../sim/general +incdir+$(BASE_DIR)/../sim/control +incdir+$(BASE_DIR)/../sim/rfnoc +incdir+$(BASE_DIR)/../lib/rfnoc

##vclean:     Cleanup Modelsim intermediate files
vclean:
	@rm -f modelsim*.log
	@rm -rf modelsim_proj
	@rm -f vivado_pid*.str
	@rm -rf work

# Use clean with :: to support allow "make clean" to work with multiple makefiles
clean:: xclean vclean

help::
	@grep -h "##" $(abspath $(lastword $(MAKEFILE_LIST))) | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

.PHONY: xsim xsim_hls xclean vsim vlint vclean clean help

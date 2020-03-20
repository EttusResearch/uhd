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

# Resolve path
EXP_DESIGN_SRCS = $(call RESOLVE_PATHS,$(DESIGN_SRCS))
EXP_SIM_SRCS    = $(call RESOLVE_PATHS,$(SIM_SRCS))
EXP_INC_SRCS    = $(call RESOLVE_PATHS,$(INC_SRCS))

# (NOQ) No quotes!
NOQ_DESIGN_SRCS := $(subst $\",,$(EXP_DESIGN_SRCS))
NOQ_SIM_SRCS    := $(subst $\",,$(EXP_SIM_SRCS))
NOQ_INC_SRCS    := $(subst $\",,$(EXP_INC_SRCS))

# Separate out VHDL
NOQ_DESIGN_VHDL := $(filter %.vhd,$(NOQ_DESIGN_SRCS))
NOQ_SIM_VHDL    := $(filter %.vhd,$(NOQ_SIM_SRCS))
NOQ_VHDL        := $(NOQ_DESIGN_VHDL) $(NOQ_SIM_VHDL)

# Separate out System Verilog
NOQ_DESIGN_SV := $(filter %.sv,$(NOQ_DESIGN_SRCS))
NOQ_SIM_SV    := $(filter %.sv,$(NOQ_SIM_SRCS))
NOQ_SV        := $(NOQ_DESIGN_SV) $(NOQ_SIM_SV)
# Fetch packages from include list to compile
NOQ_PKG_SV    := $(filter %.sv,$(NOQ_INC_SRCS))

# Seperate out Verilog
NOQ_INC_DIRS       := $(sort $(dir $(NOQ_DESIGN_SRCS) $(NOQ_SIM_SRCS) $(NOQ_INC_SRCS)))
NOQ_DESIGN_VERILOG := $(filter %.v,$(NOQ_DESIGN_SRCS))
NOQ_SIM_VERILOG    := $(filter %.v,$(NOQ_SIM_SRCS))
NOQ_VERILOG        := $(NOQ_DESIGN_VERILOG) $(NOQ_SIM_VERILOG)

# Modelsim Load libraries
MODELSIM_LIBS += unisims_ver

# Arguments for various simulators
MODELSIM_ARGS_L += $(MODELSIM_ARGS) -quiet
SVLOG_ARGS_L    += $(SVLOG_ARGS) -quiet +define+WORKING_DIR="\"${CURDIR}\""
VLOG_ARGS_L     += $(VLOG_ARGS) -quiet +define+WORKING_DIR="\"${CURDIR}\""
VHDL_ARGS_L     += $(VHDL_ARGS) -quiet

# Working directory for standalone ModelSim execution
MODELSIM_PROJ_DIR ?= modelsim_proj

# Check if we want to load the ModelSim GUI
ifeq ($(GUI), 1)
	MODELSIM_ARGS_L +=
else
	MODELSIM_ARGS_L += -c -do "run -all; quit -f"
endif

SETUP_AND_LAUNCH_SIMULATION = \
	@ \
	export VIV_SIMULATOR=$1; \
	export VIV_DESIGN_SRCS=$(EXP_DESIGN_SRCS); \
	export VIV_SIM_SRCS=$(EXP_SIM_SRCS); \
	export VIV_INC_SRCS=$(EXP_INC_SRCS); \
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

##vsim:       Run the simulation using ModelSim (via vivado)
vsim: .check_tool $(COMPLIBDIR) $(DESIGN_SRCS) $(SIM_SRCS) $(INC_SRCS)
	$(call SETUP_AND_LAUNCH_SIMULATION,Modelsim)

##modelsim:   Run the simulation using Modelsim (natively)
modelsim: .check_tool vlint
	cd $(MODELSIM_PROJ_DIR) && vsim $(MODELSIM_ARGS_L) $(foreach lib,$(MODELSIM_LIBS),-L $(lib)) $(SIM_TOP)


# NOTE: VHDL files require a correct compile order.  This script compiles files
#       in the order they are defined in $(DESIGN_SRC), then $SIM_SRC)

##vlint:      Run ModelSim compiler to lint files.
vlint: .check_tool $(COMPLIBDIR) $(DESIGN_SRCS) $(SIM_SRCS) $(INC_SRCS)
	$(shell mkdir -p ./$(MODELSIM_PROJ_DIR))
	$(file >$(MODELSIM_PROJ_DIR)/svlogarglist.txt,/* Auto generated argument file for vlog -sv */)
	$(file >>$(MODELSIM_PROJ_DIR)/svlogarglist.txt,-sv)
	$(foreach dir,$(NOQ_INC_DIRS),       $(file >>$(MODELSIM_PROJ_DIR)/svlogarglist.txt,+incdir+$(dir)))
	$(foreach src,$(NOQ_PKG_SV),         $(file >>$(MODELSIM_PROJ_DIR)/svlogarglist.txt,$(src)))
	$(foreach src,$(NOQ_SV),             $(file >>$(MODELSIM_PROJ_DIR)/svlogarglist.txt,$(src)))
	$(file >$(MODELSIM_PROJ_DIR)/vlogarglist.txt,/* Auto generated argument file for vlog */)
	$(file >>$(MODELSIM_PROJ_DIR)/vlogarglist.txt,-vlog01compat)
	$(foreach dir,$(NOQ_INC_DIRS),       $(file >>$(MODELSIM_PROJ_DIR)/vlogarglist.txt,+incdir+$(dir)))
	$(foreach src,$(NOQ_VERILOG),        $(file >>$(MODELSIM_PROJ_DIR)/vlogarglist.txt,$(src)))
	$(file >$(MODELSIM_PROJ_DIR)/vcomarglist.txt,/* Auto generated argument file for vcom */)
	$(file >>$(MODELSIM_PROJ_DIR)/vcomarglist.txt,-2008) 
	$(foreach src,$(NOQ_VHDL),$(file >>$(MODELSIM_PROJ_DIR)/vcomarglist.txt,$(src)))
ifneq ($(strip $(NOQ_SV)),)  
	@echo "*** COMPILING SYSTEM VERILOG ***"
	cd $(MODELSIM_PROJ_DIR) && vlog $(SVLOG_ARGS_L) -f svlogarglist.txt
endif
ifneq ($(strip $(NOQ_VERILOG)),)  
	@echo "*** COMPILING VERILOG ***"
	cd $(MODELSIM_PROJ_DIR) && vlog $(VLOG_ARGS_L) -f vlogarglist.txt
endif   
ifneq ($(strip $(NOQ_VHDL)),) 
	@echo "*** COMPILING VHDL ***"
	cd $(MODELSIM_PROJ_DIR) && vcom $(VHDL_ARGS_L) -f vcomarglist.txt
endif
   
##vclean:     Cleanup ModelSim intermediate files
vclean:
	@rm -f modelsim*.log
	@rm -rf $(MODELSIM_PROJ_DIR)
	@rm -f vivado_pid*.str

# Use clean with :: to support allow "make clean" to work with multiple makefiles
clean:: xclean vclean

help::
	@grep -h "##" $(abspath $(lastword $(MAKEFILE_LIST))) | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

.PHONY: xsim xsim_hls xclean vsim vlint vclean clean help

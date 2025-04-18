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
# Variables
# -------------------------------------------------------------------

ifdef SIM_COMPLIBDIR
COMPLIBDIR = $(call RESOLVE_PATH,$(SIM_COMPLIBDIR))
endif

# Get full part name, formatted for Vivado
PART_NAME=`python3 $(TOOLS_DIR)/scripts/viv_gen_part_id.py $(ARCH)/$(PART_ID)`

# Resolve path
EXP_DESIGN_SRCS = $(call RESOLVE_PATHS,$(DESIGN_SRCS))
EXP_SIM_SRCS    = $(call RESOLVE_PATHS,$(SIM_SRCS))
EXP_INC_SRCS    = $(call RESOLVE_PATHS,$(INC_SRCS))

# Working directory for native ModelSim execution
MODELSIM_PROJ_DIR ?= modelsim_proj

# -------------------------------------------------------------------
# Usage: SETUP_AND_LAUNCH_SIMULATION
# Args: $1 = Simulator Name
# -------------------------------------------------------------------

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
	export VIV_VERILOG_DEFS="UHD_FPGA_DIR=$(BASE_DIR)/../.."; \
	$(TOOLS_DIR)/scripts/launch_vivado.sh -mode $(VIVADO_MODE) -source $(call RESOLVE_PATH,$(TOOLS_DIR)/scripts/viv_sim_project.tcl) -log xsim.log -nojournal

# -------------------------------------------------------------------
# Usage: SETUP_AND_LAUNCH_VCOM
# Args: N/A
# -------------------------------------------------------------------

SETUP_AND_LAUNCH_VCOM = \
	@ \
	export VCOM_PROJ_DIR=$(MODELSIM_PROJ_DIR); \
	export VCOM_DESIGN_SRCS=$(EXP_DESIGN_SRCS); \
	export VCOM_SIM_SRCS=$(EXP_SIM_SRCS); \
	export VCOM_INC_SRCS=$(EXP_INC_SRCS); \
	export VCOM_SVLOG_ARGS="$(SVLOG_ARGS) +define+UHD_FPGA_DIR=$(BASE_DIR)/../.."; \
	export VCOM_VLOG_ARGS="$(VLOG_ARGS) +define+UHD_FPGA_DIR=$(BASE_DIR)/../.."; \
	export VCOM_VHDL_ARGS="$(VHDL_ARGS)"; \
	export VCOM_MODELSIM_INI="$(MODELSIM_INI)"; \
	$(TOOLS_DIR)/scripts/launch_vcom.sh

# -------------------------------------------------------------------
# Usage: SETUP_AND_LAUNCH_MODELSIM
# Args: N/A
# -------------------------------------------------------------------

SETUP_AND_LAUNCH_MODELSIM = \
	@ \
	export MSIM_PROJ_DIR=$(MODELSIM_PROJ_DIR); \
	export MSIM_SIM_TOP="$(SIM_TOP)"; \
	export MSIM_ARGS="$(MODELSIM_ARGS)"; \
	export MSIM_LIBS="$(MODELSIM_LIBS)"; \
	export MSIM_MODE=$(VIVADO_MODE); \
	export MSIM_MODELSIM_INI="$(MODELSIM_INI)"; \
	export MSIM_VARIANT=legacy; \
	$(TOOLS_DIR)/scripts/launch_modelsim.sh

# -------------------------------------------------------------------
# Usage: SETUP_AND_LAUNCH_QUESTASIM
# Args: N/A
# -------------------------------------------------------------------

SETUP_AND_LAUNCH_QUESTASIM = \
	@ \
	export MSIM_GUI=visualizer; \
	export MSIM_PROJ_DIR=$(MODELSIM_PROJ_DIR); \
	export MSIM_SIM_TOP="$(SIM_TOP)"; \
	export MSIM_ARGS="$(MODELSIM_ARGS)"; \
	export MSIM_LIBS="$(MODELSIM_LIBS)"; \
	export MSIM_MODE=$(VIVADO_MODE); \
	export MSIM_MODELSIM_INI="$(MODELSIM_INI)"; \
	export MSIM_VARIANT=questa; \
	$(TOOLS_DIR)/scripts/launch_modelsim.sh

.SECONDEXPANSION:

##ip:         Generate the IP required for this simulation
ip: $(DESIGN_SRCS)

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

##vsim:       Run the simulation using ModelSim from inside Vivado
vsim: .check_tool $(COMPLIBDIR) $(DESIGN_SRCS) $(SIM_SRCS) $(INC_SRCS)
	$(call SETUP_AND_LAUNCH_SIMULATION,Modelsim)

##qsim:       Run the simulation using Questa
qsim: .check_tool vlint
	$(call SETUP_AND_LAUNCH_QUESTASIM)

##msim:       Run the simulation using ModelSim
msim: .check_tool vlint
	$(call SETUP_AND_LAUNCH_MODELSIM)

# Alias for msim (for backwards compatibility)
modelsim: msim

# NOTE: VHDL files require a correct compile order.  This script compiles files
#       in the order they are defined in $(DESIGN_SRC), then $SIM_SRC)

##vcom:       Run ModelSim/Questa compiler to compile HDL files
vcom: .check_tool $(DESIGN_SRCS) $(SIM_SRCS) $(INC_SRCS)
	$(call SETUP_AND_LAUNCH_VCOM)

# Alias for vcom (for backwards compatibility)
vlint: vcom

##vclean:     Cleanup ModelSim/Questa intermediate files
vclean:
	@rm -f modelsim*.log
	@rm -rf $(MODELSIM_PROJ_DIR)
	@rm -f vivado_pid*.str

# Use clean with :: to support allow "make clean" to work with multiple makefiles
clean:: xclean vclean

help::
	@grep -h "##" $(abspath $(lastword $(MAKEFILE_LIST))) | grep -v "\"##\"" | sed -e 's/\\$$//' | sed -e 's/##//'

.PHONY: ip xsim xsim_hls xclean vsim vlint vclean clean help

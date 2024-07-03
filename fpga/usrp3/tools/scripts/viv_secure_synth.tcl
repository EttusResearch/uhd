#
# Copyright 2024 Ettus Research, a National Instruments Brand
#

source $::env(VIV_TOOLS_DIR)/scripts/viv_utils.tcl
source $::env(VIV_TOOLS_DIR)/scripts/viv_strategies.tcl

# STEP#1: Create project, add sources, refresh IP
vivado_utils::initialize_project

# STEP#2: Run synthesis
vivado_utils::synthesize_design -mode out_of_context
vivado_utils::generate_post_synth_reports

# STEP#3: Generate encrypted netlist
vivado_utils::export_encrypted_netlist

# Cleanup
vivado_utils::close_batch_project

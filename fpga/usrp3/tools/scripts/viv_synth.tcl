#
# Copyright 2019 Ettus Research, a National Instruments Brand
#

source $::env(VIV_TOOLS_DIR)/scripts/viv_utils.tcl
source $::env(VIV_TOOLS_DIR)/scripts/viv_strategies.tcl

# STEP#1: Create project, add sources, refresh IP
vivado_utils::initialize_project

# STEP#2: Run synthesis 
vivado_utils::synthesize_design
vivado_utils::generate_post_synth_reports

# Cleanup
vivado_utils::close_batch_project

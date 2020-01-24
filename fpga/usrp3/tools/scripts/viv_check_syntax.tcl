#
# Copyright 2018 Ettus Research
#

source $::env(VIV_TOOLS_DIR)/scripts/viv_utils.tcl

# STEP#1: Create project, add sources, refresh IP
vivado_utils::initialize_project

# STEP#2: Run elaboration
vivado_utils::check_design

# Cleanup
vivado_utils::close_batch_project

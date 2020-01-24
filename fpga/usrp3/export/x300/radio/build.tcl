# Copyright 2015 Ettus Research

source $::env(VIV_TOOLS_DIR)/scripts/viv_utils.tcl

vivado_utils::initialize_project
vivado_utils::synthesize_design
vivado_utils::write_netlist_outputs
vivado_utils::close_batch_project
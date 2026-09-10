#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

source $::env(VIV_TOOLS_DIR)/scripts/viv_utils.tcl
source $::env(VIV_TOOLS_DIR)/scripts/viv_strategies.tcl

# STEP#1: Create project, add sources, refresh IP
vivado_utils::initialize_project

# STEP#2: Run synthesis
vivado_utils::synthesize_design
vivado_utils::generate_post_synth_reports
vivado_utils::exit_if_synth_only

# STEP#3: Run implementation strategy
set strategy [vivado_strategies::get_impl_preset "Performance_ExplorePostRoutePhysOpt"]
# Enable incremental build
dict set strategy "implementation.incremental" $::env(VIV_INCR_BUILD)
# Vivado has been underestimating routing delays.
dict set strategy "place_design.directive" "ExtraNetDelay_high"
vivado_strategies::implement_design $strategy

# STEP#4: Generate reports
vivado_utils::generate_post_route_reports

# STEP#5: Generate a bitstream, netlist and debug probes
set_property BITSTREAM.CONFIG.USR_ACCESS TIMESTAMP [get_designs *]
set byte_swap_bin 1
vivado_utils::write_implementation_outputs $byte_swap_bin

# Cleanup
vivado_utils::close_batch_project

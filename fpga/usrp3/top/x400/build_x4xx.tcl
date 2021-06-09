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

# STEP#3: Run implementation strategy
set strategy [vivado_strategies::get_impl_preset "Performance_ExplorePostRoutePhysOpt"]
# Turn up uncertainty on 100Gb clocks(-quiet so if it fails because the clocks don't exist, it won't error)
set_clock_uncertainty 0.5 -quiet -setup [get_clocks txoutclk_out*]
# Vivado has been underestimating routing delays.
dict set strategy "place_design.directive"                  "ExtraNetDelay_high"
# Turn down uncertainty on 100Gb clocks
dict set strategy "route_design.pre_hook" {set_clock_uncertainty 0.0 -quiet -setup [get_clocks txoutclk_out*]}
vivado_strategies::implement_design $strategy

# STEP#4: Generate reports
vivado_utils::generate_post_route_reports

# STEP#5: Generate a bitstream, netlist and debug probes
set_property BITSTREAM.CONFIG.USR_ACCESS TIMESTAMP [get_designs *]
set byte_swap_bin 1
vivado_utils::write_implementation_outputs $byte_swap_bin

# Cleanup
vivado_utils::close_batch_project

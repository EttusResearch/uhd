#
# Copyright 2014-2015 Ettus Research
#

source $::env(VIV_TOOLS_DIR)/scripts/viv_utils.tcl
source $::env(VIV_TOOLS_DIR)/scripts/viv_strategies.tcl

# STEP#1: Create project, add sources, refresh IP
vivado_utils::initialize_project

# STEP#2: Run synthesis 
vivado_utils::synthesize_design
vivado_utils::generate_post_synth_reports

# STEP#3: Run implementation strategy
set x3xx_strategy [dict create]
dict set x3xx_strategy "opt_design.is_enabled"                   1
dict set x3xx_strategy "opt_design.directive"                    "NoBramPowerOpt"
dict set x3xx_strategy "post_opt_power_opt_design.is_enabled"    0
dict set x3xx_strategy "place_design.directive"                  "ExtraNetDelay_high"
dict set x3xx_strategy "post_place_power_opt_design.is_enabled"  0
dict set x3xx_strategy "post_place_phys_opt_design.is_enabled"   1
dict set x3xx_strategy "post_place_phys_opt_design.directive"    "AggressiveExplore"
dict set x3xx_strategy "route_design.directive"                  "Explore"
dict set x3xx_strategy "route_design.more_options"               "-tns_cleanup"
dict set x3xx_strategy "post_route_phys_opt_design.is_enabled"   1
dict set x3xx_strategy "post_route_phys_opt_design.directive"    "Explore"
vivado_strategies::implement_design $x3xx_strategy

# STEP#4: Generate reports
vivado_utils::generate_post_route_reports

# STEP#5: Generate a bitstream, netlist and debug probes

# STC3 Requirement: Disable waiting for DCI Match
set_property BITSTREAM.STARTUP.MATCH_CYCLE  NoWait  [get_designs *]
# STC3 Requirement: No bitstream compression
set_property BITSTREAM.GENERAL.COMPRESS     False   [get_designs *]
# Use 6MHz clock to configure bitstream
set_property BITSTREAM.CONFIG.CONFIGRATE    6       [get_designs *]

vivado_utils::write_implementation_outputs

# Cleanup
vivado_utils::close_batch_project

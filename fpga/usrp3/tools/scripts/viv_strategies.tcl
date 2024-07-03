#
# Copyright 2015 Ettus Research
#

# ---------------------------------------------------
# Create namespace and initialize global parameters
# ---------------------------------------------------
namespace eval ::vivado_strategies {
    # Export commands
    namespace export \
        get_preset \
        implement_design \
        check_strategy \
        print_strategy

    # Required environment variables
    variable g_output_dir  $::env(VIV_OUTPUT_DIR)
}

# ---------------------------------------------------
# Return a preset strategy with the most commonly used options
# ---------------------------------------------------
proc ::vivado_strategies::get_impl_preset {preset} {
    set strategy [dict create]
    switch -nocase $preset {
        "Default" {
            dict set strategy "implementation.incremental"              0
            dict set strategy "opt_design.is_enabled"                   1
            dict set strategy "opt_design.directive"                    "Default"
            dict set strategy "post_opt_power_opt_design.is_enabled"    0
            dict set strategy "place_design.directive"                  "Default"
            dict set strategy "post_place_power_opt_design.is_enabled"  0
            dict set strategy "post_place_phys_opt_design.is_enabled"   0
            dict set strategy "post_place_phys_opt_design.directive"    "Default"
            dict set strategy "route_design.directive"                  "Default"
            dict set strategy "route_design.more_options"               ""
            dict set strategy "post_route_phys_opt_design.is_enabled"   0
            dict set strategy "post_route_phys_opt_design.directive"    "Default"
        }
        "Performance_Explore" {
            dict set strategy "implementation.incremental"              0
            dict set strategy "opt_design.is_enabled"                   1
            dict set strategy "opt_design.directive"                    "Explore"
            dict set strategy "post_opt_power_opt_design.is_enabled"    0
            dict set strategy "place_design.directive"                  "Explore"
            dict set strategy "post_place_power_opt_design.is_enabled"  0
            dict set strategy "post_place_phys_opt_design.is_enabled"   1
            dict set strategy "post_place_phys_opt_design.directive"    "Explore"
            dict set strategy "route_design.directive"                  "Explore"
            dict set strategy "route_design.more_options"               ""
            dict set strategy "post_route_phys_opt_design.is_enabled"   0
            dict set strategy "post_route_phys_opt_design.directive"    "Explore"
        }
        "Performance_ExplorePostRoutePhysOpt" {
            dict set strategy "implementation.incremental"              0
            dict set strategy "opt_design.is_enabled"                   1
            dict set strategy "opt_design.directive"                    "Explore"
            dict set strategy "post_opt_power_opt_design.is_enabled"    0
            dict set strategy "place_design.directive"                  "Explore"
            dict set strategy "post_place_power_opt_design.is_enabled"  0
            dict set strategy "post_place_phys_opt_design.is_enabled"   1
            dict set strategy "post_place_phys_opt_design.directive"    "Explore"
            dict set strategy "route_design.directive"                  "Explore"
            dict set strategy "route_design.more_options"               "-tns_cleanup"
            dict set strategy "post_route_phys_opt_design.is_enabled"   1
            dict set strategy "post_route_phys_opt_design.directive"    "Explore"
        }
    }
    return $strategy
}

# ---------------------------------------------------
# Execute the specified implementation strategy
# ---------------------------------------------------
proc ::vivado_strategies::implement_design {strategy} {
    variable g_output_dir

    # Check strategy for validity and print
    vivado_strategies::check_strategy $strategy
    puts "BUILDER: Running implementation strategy with:"
    vivado_strategies::print_strategy $strategy

    # Optimize the current netlist.
    # This will perform the retarget, propconst, sweep and bram_power_opt optimizations by default.
    if [dict get $strategy "opt_design.is_enabled"] {
        set opt_dir [dict get $strategy "opt_design.directive"]
        opt_design -directive $opt_dir
    }

    # Reading former post-route checkpoint for incremental implementation.
    # This will set all consecutive operations into incremental operation mode in case there
    # is a post_route checkpoint from a previous run.
    if [dict get $strategy "implementation.incremental"] {
        set impl_checkpoint $g_output_dir/post_route.dcp
        if {[file exists $impl_checkpoint]} {
            # read incremental checkpoint
            read_checkpoint -incremental $impl_checkpoint
        }
    }

    # Optimize dynamic power using intelligent clock gating after optimization
    if [dict get $strategy "post_opt_power_opt_design.is_enabled"] {
        power_opt_design
    }

    # Automatically place ports and leaf-level instances
    set pla_dir [dict get $strategy "place_design.directive"]
    place_design -directive $pla_dir

    # Optimize dynamic power using intelligent clock gating after placement
    if [dict get $strategy "post_place_power_opt_design.is_enabled"] {
        power_opt_design
    }

    # Optimize the current placed netlist
    if [dict get $strategy "post_place_phys_opt_design.is_enabled"] {
        set pp_physopt_dir [dict get $strategy "post_place_phys_opt_design.directive"]
        phys_opt_design -directive $pp_physopt_dir
    }

    # Option to run commands before route
    if {[dict exist $strategy "route_design.pre_hook"]} {
      set pre_route_cmds [dict get $strategy "route_design.pre_hook"]
      eval $pre_route_cmds
    }

    # Route the current design
    set rt_dir [dict get $strategy "route_design.directive"]
    puts "BUILDER: Choosing routing directive: $rt_dir"
    if {[dict get $strategy "route_design.more_options"] eq ""} {
        route_design -directive $rt_dir
    } else {
        set rt_more [dict get $strategy "route_design.more_options"]
        puts "BUILDER: Choosing additional routing options: $rt_more"
        route_design -directive $rt_dir $rt_more
    }

    # Optimize the current routed netlist.
    if [dict get $strategy "post_route_phys_opt_design.is_enabled"] {
        set pr_physopt_dir [dict get $strategy "post_route_phys_opt_design.directive"]
        phys_opt_design -directive $pr_physopt_dir
    }
}

# ---------------------------------------------------
# Sanity-check the specified strategy
# ---------------------------------------------------
proc ::vivado_strategies::check_strategy {strategy} {
    set strategy_options [dict keys $strategy]
    set required_options {\
        implementation.incremental \
        opt_design.is_enabled \
        opt_design.directive \
        post_opt_power_opt_design.is_enabled \
        place_design.directive \
        post_place_power_opt_design.is_enabled \
        post_place_phys_opt_design.is_enabled \
        post_place_phys_opt_design.directive \
        route_design.directive \
        post_route_phys_opt_design.is_enabled \
        post_route_phys_opt_design.directive \
    }

    set invalid 0
    foreach req $required_options {
        if [expr [lsearch $strategy_options $req] < 0] {
            puts "BUILDER: ERROR: Invalid strategy. Missing option $req"
            set invalid 1
        }
    }
    if $invalid {
        error "Strategy check failed!"
    }
}

# ---------------------------------------------------
# Print strategy parameters to the console
# ---------------------------------------------------
proc ::vivado_strategies::print_strategy {strategy} {
    foreach opt [dict keys $strategy] {
        set val [dict get $strategy $opt]
        puts "         * $opt = $val"
    }
}



#
# Copyright 2014-2015 Ettus Research
#

# ---------------------------------------------------
# Create namespace and initialize global parameters
# ---------------------------------------------------
namespace eval ::vivado_utils {
    # Export commands
    namespace export \
        initialize_project \
        synthesize_design \
        export_synth_netlist \
        separate_encrypted \
        export_encrypted_netlist \
        check_design \
        generate_post_synth_reports \
        generate_post_place_reports \
        generate_post_route_reports \
        write_implementation_outputs \
        get_top_module \
        get_part_name \
        get_vivado_mode

    # Required environment variables
    variable g_tools_dir    $::env(VIV_TOOLS_DIR)
    variable g_top_module   $::env(VIV_TOP_MODULE)
    variable g_part_name    $::env(VIV_PART_NAME)
    variable g_output_dir   $::env(VIV_OUTPUT_DIR)
    variable g_source_files $::env(VIV_DESIGN_SRCS)
    variable g_vivado_mode  $::env(VIV_MODE)
    variable g_project_save $::env(VIV_PROJECT)
    variable g_secure_key   $::env(VIV_SECURE_KEY)

    # Optional environment variables
    variable g_verilog_defs ""
    if { [info exists ::env(VIV_VERILOG_DEFS) ] } {
        set g_verilog_defs  $::env(VIV_VERILOG_DEFS)
    }
    variable g_include_dirs ""
    if { [info exists ::env(VIV_INCLUDE_DIRS) ] } {
        set g_include_dirs  $::env(VIV_INCLUDE_DIRS)
    }
}

# ---------------------------------------------------
# Create a new project in memory and add source files
# ---------------------------------------------------
proc ::vivado_utils::initialize_project { {save_to_disk 0} } {
    variable g_top_module
    variable g_part_name
    variable g_output_dir
    variable g_source_files
    variable g_project_save

    variable bd_files ""

    file delete -force $g_output_dir/build.rpt

    if {$save_to_disk == 1 || $g_project_save == 1} {
        puts "BUILDER: Creating Vivado project ${g_top_module}_project.xpr for part $g_part_name"
        create_project -force -part $g_part_name ${g_top_module}_project
    } else {
        puts "BUILDER: Creating Vivado project in memory for part $g_part_name"
        create_project -in_memory -part $g_part_name
    }

    # Expand directories to include their contents (needed for HLS outputs)
    foreach src_file $g_source_files {
        if [expr [file isdirectory $src_file] == 1] {
            puts "BUILDER: Expanding Directory : $src_file"
            set dir_contents [glob $src_file/*.*]
            append g_source_files " " $dir_contents
        }
    }

    foreach src_file $g_source_files {
        set src_ext [file extension $src_file ]
        if [expr [lsearch {.vhd .vhdl} $src_ext] >= 0] {
            puts "BUILDER: Adding VHDL: $src_file"
            read_vhdl -library work $src_file
        } elseif [expr [lsearch {.v .vh} $src_ext] >= 0] {
            puts "BUILDER: Adding Verilog: $src_file"
            read_verilog $src_file
        } elseif [expr [lsearch {.vp} $src_ext] >= 0] {
            puts "BUILDER: Adding encrypted Verilog: $src_file"
            read_verilog $src_file
        } elseif [expr [lsearch {.sv .svh} $src_ext] >= 0] {
            puts "BUILDER: Adding SystemVerilog: $src_file"
            read_verilog -sv $src_file
        } elseif [expr [lsearch {.xdc} $src_ext] >= 0] {
            puts "BUILDER: Adding XDC: $src_file"
            read_xdc $src_file
        } elseif [expr [lsearch {.sdc} $src_ext] >= 0] {
            puts "BUILDER: Adding SDC: $src_file"
            read_xdc $src_file
        } elseif [expr [lsearch {.xci} $src_ext] >= 0] {
            puts "BUILDER: Adding IP: $src_file"
            read_ip $src_file
            set_property generate_synth_checkpoint true [get_files $src_file]
        } elseif [expr [lsearch {.ngc .edif .edf} $src_ext] >= 0] {
            puts "BUILDER: Adding Netlist: $src_file"
            read_edif $src_file
        } elseif [expr [lsearch {.bd} $src_ext] >= 0] {
            puts "BUILDER: Adding Block Design to list (added after IP regeneration): $src_file"
            append bd_files "$src_file "
        } elseif [expr [lsearch {.bxml} $src_ext] >= 0] {
            puts "BUILDER: Adding Block Design XML to list (added after IP regeneration): $src_file"
            append bd_files "$src_file "
        } elseif [expr [lsearch {.dat} $src_ext] >= 0] {
            puts "BUILDER: Adding Data File: $src_file"
            add_files $src_file
        } else {
            puts "BUILDER: \[WARNING\] File ignored!!!: $src_file"
        }
    }

    # The 'synth_ip [get_ips *]' step causes builds in Windows to recompile various
    # pieces of the IP. This is time-consuming and unnecessary behavior, thus is removed.
    # These steps are redundant anyway since the IP builder performs both of them.
    # puts "BUILDER: Refreshing IP"
    # generate_target all [get_ips *]
    # synth_ip [get_ips *]

    #might seem silly, but we need to add the bd files after the ip regeneration.
    foreach file $bd_files {
        puts "BUILDER: Adding file from Block Design list: $file"
        add_files -norecurse $file
    }

    puts "BUILDER: Setting $g_top_module as the top module"
    set_property top $g_top_module [current_fileset]
}

# ---------------------------------------------------
# Synthesize design (Shortcut for Vivado's synth_design)
# ---------------------------------------------------
proc ::vivado_utils::synthesize_design {args} {
    variable g_top_module
    variable g_part_name
    variable g_verilog_defs
    variable g_include_dirs

    set vdef_args ""
    foreach vdef $g_verilog_defs {
        set vdef_args [concat $vdef_args "-verilog_define $vdef"]
    }
    set incdir_args ""
    if { [string compare $g_include_dirs ""] != 0 } {
        set incdir_args "-include_dirs $g_include_dirs"
    }

    set synth_cmd "synth_design -top $g_top_module -part $g_part_name"
    set synth_cmd [concat $synth_cmd $vdef_args]
    set synth_cmd [concat $synth_cmd $incdir_args]
    set synth_cmd [concat $synth_cmd $args]
    puts "BUILDER: Synthesizing design"
    eval $synth_cmd
}

# ---------------------------------------------------
# Generate netlist from Synthesis
# ---------------------------------------------------
proc ::vivado_utils::export_synth_netlist { {suffix ""} } {
    variable g_output_dir
    variable g_top_module

    puts "BUILDER: Writing EDIF netlist for $g_top_module"
    set filename ${g_output_dir}/${g_top_module}
    if { [expr [string length $suffix] > 0] } {
        set filename ${filename}_${suffix}
    }
    write_edif -security_mode all -force ${filename}.edf
    write_verilog -mode synth_stub -force -file ${filename}_stub.v
}

# ---------------------------------------------------
# Extract encrypted sections from a file
# ---------------------------------------------------
proc ::vivado_utils::separate_encrypted {input_filename unencrypted_filename encrypted_filename} {
    set protected_sections ""
    set unprotected_sections ""
    set inside_protected_section 0

    # Read each line from the input file and separate it into protected and
    # unprotected sections.
    set input_file [open $input_filename r]
    while {[gets $input_file line] >= 0} {
        # Check if we are inside a protected section
        if {[string match "*`pragma protect begin_protected*" $line]} {
            set inside_protected_section 1
        }

        # Append the line to the appropriate section
        if {$inside_protected_section} {
            append protected_sections "$line\n"
        } else {
            append unprotected_sections "$line\n"
        }

        if {[string match "*`pragma protect end_protected*" $line]} {
            set inside_protected_section 0
        }
    }
    close $input_file

    # Write the extracted portions to files
    set encrypted_file [open $encrypted_filename w]
    puts $encrypted_file $protected_sections
    close $encrypted_file
    set unencrypted_file [open $unencrypted_filename w]
    puts $unencrypted_file $unprotected_sections
    close $unencrypted_file
}

# ---------------------------------------------------------
# Generate an IEEE-1735 encrypted netlist in a single file
# ---------------------------------------------------------
proc ::vivado_utils::export_encrypted_netlist { {suffix ""} } {
    variable g_output_dir
    variable g_top_module
    variable g_secure_key

    puts "BUILDER: Writing encrypted netlist for $g_top_module"
    set filename ${g_output_dir}/${g_top_module}
    if { [expr [string length $suffix] > 0] } {
        set filename ${filename}_${suffix}
    }

    # Uniquify the module names to prevent collisions when the netlist is used
    rename_ref -prefix_all $g_top_module

    # Write the design to a Verilog netlist
    write_verilog -force ${filename}_netlist.v
    puts "BUILDER: Unencrypted netlist written to ${filename}_netlist.v"
    write_xdc -force ${filename}.xdc
    puts "BUILDER: Constraints written to ${filename}.xdc"
    if {$g_secure_key ne ""} {
        # The netlist may have parts that are already encrypted and Vivado doesn't
        # let us encrypt stuff that's already encrypted, so we need to split it up.
        vivado_utils::separate_encrypted ${filename}_netlist.v ${filename}_netlist_user.v ${filename}_netlist_other.vp
        # Encrypt the parts that aren't already encrypted
        encrypt -key ${g_secure_key} -lang verilog -ext .vp ${filename}_netlist_user.v
        # Merge the two encrypted files
        set combined_netlists [read [open ${filename}_netlist_user.vp r]][read [open ${filename}_netlist_other.vp r]]
        set combined_netlists_file [open ${filename}.vp w]
        puts $combined_netlists_file $combined_netlists
        close $combined_netlists_file
        puts "BUILDER: Encrypted netlist written to ${filename}.vp"
    }
}

# ---------------------------------------------------
# Check design (Shortcut for Vivado's synth_design -rtl)
# ---------------------------------------------------
proc ::vivado_utils::check_design {args} {
    variable g_top_module
    variable g_part_name
    variable g_verilog_defs
    variable g_include_dirs

    set vdef_args ""
    foreach vdef $g_verilog_defs {
        set vdef_args [concat $vdef_args "-verilog_define $vdef"]
    }
    set incdir_args ""
    if { [string compare $g_include_dirs ""] != 0 } {
        set incdir_args "-include_dirs $g_include_dirs"
    }

    set synth_cmd "synth_design -top $g_top_module -part $g_part_name -rtl -rtl_skip_ip -rtl_skip_constraints"
    set synth_cmd [concat $synth_cmd $vdef_args]
    set synth_cmd [concat $synth_cmd $incdir_args]
    set synth_cmd [concat $synth_cmd $args]
    puts "BUILDER: Checking syntax and elaborating design"
    eval $synth_cmd
}

# ---------------------------------------------------
# Generate post synthesis reports and checkpoint
# ---------------------------------------------------
proc ::vivado_utils::generate_post_synth_reports {} {
    variable g_output_dir

    puts "BUILDER: Writing post-synthesis checkpoint"
    write_checkpoint -force $g_output_dir/post_synth
    puts "BUILDER: Writing post-synthesis reports"
    report_utilization -file $g_output_dir/post_synth_util.rpt
    report_utilization -hierarchical -file $g_output_dir/post_synth_util_hier.rpt
    report_drc -ruledeck methodology_checks -file $g_output_dir/methodology.rpt
    report_high_fanout_nets -file $g_output_dir/high_fanout_nets.rpt
}

# ---------------------------------------------------
# Generate post placement reports and checkpoint
# ---------------------------------------------------
proc ::vivado_utils::generate_post_place_reports {} {
    variable g_output_dir

    puts "BUILDER: Writing post-placement checkpoint"
    write_checkpoint -force $g_output_dir/post_place
    puts "BUILDER: Writing post-placement reports"
    report_clock_utilization -file $g_output_dir/clock_util.rpt
    report_utilization -file $g_output_dir/post_place_util.rpt
    report_utilization -hierarchical -file $g_output_dir/post_place_util_hier.rpt
    report_timing -sort_by group -max_paths 5 -path_type summary -file $g_output_dir/post_place_timing.rpt
}

# ---------------------------------------------------
# Generate post route reports and checkpoint
# ---------------------------------------------------
proc ::vivado_utils::generate_post_route_reports {} {
    variable g_output_dir

    puts "BUILDER: Writing post-route checkpoint"
    write_checkpoint -force $g_output_dir/post_route
    puts "BUILDER: Writing post-route reports"
    if {[file exists "$g_output_dir/clock_util.rpt"] == 0} {
        report_clock_utilization -file $g_output_dir/clock_util.rpt
    }
    report_timing_summary -file $g_output_dir/post_route_timing_summary.rpt
    report_utilization -file $g_output_dir/post_route_util.rpt
    report_utilization -hierarchical -file $g_output_dir/post_route_util_hier.rpt
    report_power -file $g_output_dir/post_route_power.rpt
    report_drc -file $g_output_dir/post_imp_drc.rpt
    report_timing -sort_by group -max_paths 10 -path_type summary -file $g_output_dir/post_route_timing.rpt
}

# ---------------------------------------------------
# Export implementation
# ---------------------------------------------------
proc ::vivado_utils::check_timing_report {} {
    variable g_output_dir
    set s "(Implementation outputs were still generated)"

    if {! [string match -nocase {*timing constraints are met*} [read [open $g_output_dir/build.rpt]]]} {
        send_msg_id {Builder 0-0} error "The design did not satisfy timing constraints. ${s}"
    }
    if {! [string match -nocase {*checking no_clock (0)*} [read [open $g_output_dir/build.rpt]]]} {
        send_msg_id {Builder 0-0} error "There are register/latch pins with no clock driven by root clock pin. ${s}"
    }
    if {! [string match -nocase {*checking constant_clock (0)*} [read [open $g_output_dir/build.rpt]]]} {
        send_msg_id {Builder 0-0} error "There are register/latch pins with constant clock. ${s}"
    }
    if {! [string match -nocase {*checking multiple_clock (0)*} [read [open $g_output_dir/build.rpt]]]} {
        send_msg_id {Builder 0-0} error "There are register/latch pins with multiple clocks. ${s}"
    }
    if {! [string match -nocase {*checking generated_clocks (0)*} [read [open $g_output_dir/build.rpt]]]} {
        send_msg_id {Builder 0-0} error "There are generated clocks that are not connected to a clock source. ${s}"
    }
    if {! [string match -nocase {*checking loops (0)*} [read [open $g_output_dir/build.rpt]]]} {
        send_msg_id {Builder 0-0} error "There are combinational loops in the design. ${s}"
    }
    if {! [string match -nocase {*checking latch_loops (0)*} [read [open $g_output_dir/build.rpt]]]} {
        send_msg_id {Builder 0-0} error "There are combinational latch loops in the design. ${s}"
    }
}

proc ::vivado_utils::write_implementation_outputs { {byte_swap_bin 0} } {
    variable g_output_dir
    variable g_top_module
    variable g_tools_dir

    puts "BUILDER: Writing implementation netlist and XDC"
    write_verilog -force $g_output_dir/${g_top_module}_impl_netlist.v
    write_xdc -no_fixed_only -force $g_output_dir/${g_top_module}_impl.xdc
    puts "BUILDER: Writing bitfile"
    write_bitstream -force $g_output_dir/${g_top_module}.bit
    puts "BUILDER: Writing config bitstream"
    set binsize [expr [file size $g_output_dir/${g_top_module}.bit]/(1024*1024)]
    set binsize_pow2 [expr {int(pow(2,ceil(log($binsize)/log(2))))}]
    set bin_iface [expr $byte_swap_bin?"SMAPx32":"SMAPx8"]
    write_cfgmem -force -quiet -interface $bin_iface -format BIN -size $binsize_pow2 -disablebitswap -loadbit "up 0x0 $g_output_dir/${g_top_module}.bit" $g_output_dir/${g_top_module}.bin
    puts "BUILDER: Writing debug probes"
    write_debug_probes -force $g_output_dir/${g_top_module}.ltx
    puts "BUILDER: Writing export report"
    report_utilization -omit_locs -file $g_output_dir/build.rpt
    report_timing_summary -no_detailed_paths -file $g_output_dir/build.rpt -append
    vivado_utils::check_timing_report
}

proc ::vivado_utils::write_netlist_outputs { {suffix ""} } {
    variable g_output_dir
    variable g_top_module

    puts "BUILDER: Writing EDIF netlist and XDC"
    set filename ${g_output_dir}/${g_top_module}
    if { [expr [string length $suffix] > 0] } {
        set filename ${filename}_${suffix}
    }
    write_edif -force ${filename}.edf
    write_xdc -no_fixed_only -force ${filename}.xdc
    puts "BUILDER: Writing export report"
    report_utilization -omit_locs -file $g_output_dir/build.rpt
    report_timing_summary -no_detailed_paths -file $g_output_dir/build.rpt -append
    vivado_utils::check_timing_report
}

# ---------------------------------------------------
# Close project
# ---------------------------------------------------
proc ::vivado_utils::close_batch_project {} {
    variable g_vivado_mode

    if [string equal $g_vivado_mode "batch"] {
        puts "BUILDER: Closing project"
        close_project
    } else {
        puts "BUILDER: In GUI mode. Leaving project open."
    }
}

# ---------------------------------------------------
# Get state variables
# ---------------------------------------------------
proc ::vivado_utils::get_top_module {} {
    variable g_top_module
    return $g_top_module
}

proc ::vivado_utils::get_part_name {} {
    variable g_part_name
    return $g_part_name
}

proc ::vivado_utils::get_vivado_mode {} {
    variable g_vivado_mode
    return $g_vivado_mode
}

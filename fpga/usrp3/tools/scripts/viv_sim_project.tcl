#
# Copyright 2014 Ettus Research
#

# ---------------------------------------
# Gather all external parameters
# ---------------------------------------
set simulator       $::env(VIV_SIMULATOR)
set design_srcs     $::env(VIV_DESIGN_SRCS)
set sim_srcs        $::env(VIV_SIM_SRCS)
set inc_srcs        $::env(VIV_INC_SRCS)
set sim_top         $::env(VIV_SIM_TOP)
set part_name       $::env(VIV_PART_NAME)
set sim_runtime     $::env(VIV_SIM_RUNTIME)
set sim_fast        $::env(VIV_SIM_FAST)
set vivado_mode     $::env(VIV_MODE)
set verilog_defs    $::env(VIV_VERILOG_DEFS)
set working_dir     [pwd]

set sim_fileset "sim_1"
set project_name "[string tolower $simulator]_proj"

if [info exists ::env(VIV_SIM_COMPLIBDIR) ] {
    set sim_complibdir  $::env(VIV_SIM_COMPLIBDIR)
    if [expr [file isdirectory $sim_complibdir] == 0] {
        set sim_complibdir  ""
    }
} else {
    set sim_complibdir  ""
}
if [expr ([string equal $simulator "XSim"] == 0) && ([string length $sim_complibdir] == 0)] {
    puts "BUILDER: \[ERROR\]: Could not resolve the location for the compiled simulation libraries."
    puts "                  Please build libraries for chosen simulator and set the env or"
    puts "                  makefile variable SIM_COMPLIBDIR to point to the location."
    exit 1
}

# ---------------------------------------
# Vivado Commands
# ---------------------------------------
puts "BUILDER: Creating Vivado simulation project part $part_name"
create_project -part $part_name -force $project_name/$project_name

# Expand directories to include their contents (needed for HLS outputs)
foreach src_file $design_srcs {
    if [expr [file isdirectory $src_file] == 1] {
        puts "BUILDER: Expanding Directory : $src_file"
        set dir_contents [glob $src_file/*.*]
        append design_srcs " " $dir_contents
    }
}

foreach src_file $design_srcs {
    set src_ext [file extension $src_file ]
    if [expr [lsearch {.vhd .vhdl} $src_ext] >= 0] {
        puts "BUILDER: Adding VHDL    : $src_file"
        read_vhdl $src_file
    } elseif [expr [lsearch {.v .vh} $src_ext] >= 0] {
        puts "BUILDER: Adding Verilog : $src_file"
        read_verilog $src_file
    } elseif [expr [lsearch {.sv .svh} $src_ext] >= 0] {
        puts "BUILDER: Adding SVerilog: $src_file"
        read_verilog -sv $src_file
    } elseif [expr [lsearch {.xdc} $src_ext] >= 0] {
        puts "BUILDER: Adding XDC     : $src_file"
        read_xdc $src_file
    } elseif [expr [lsearch {.xci} $src_ext] >= 0] {
        puts "BUILDER: Adding IP      : $src_file"
        read_ip $src_file
    } elseif [expr [lsearch {.ngc .edif} $src_ext] >= 0] {
        puts "BUILDER: Adding Netlist : $src_file"
        read_edif $src_file
    } elseif [expr [lsearch {.dat} $src_ext] >= 0] {
        puts "BUILDER: Adding Data File : $src_file"
        add_files -fileset $sim_fileset -norecurse $src_file
    } elseif [expr [lsearch {.bd} $src_ext] >= 0] {
        puts "BUILDER: Adding Block Diagram: $src_file"
        add_files -norecurse $src_file
    } elseif [expr [lsearch {.bxml} $src_ext] >= 0] {
        puts "BUILDER: Adding Block Diagram XML: $src_file"
        add_files -norecurse $src_file
    } else {
        puts "BUILDER: \[WARNING\] File ignored!!!: $src_file"
    }
}

foreach sim_src $sim_srcs {
    puts "BUILDER: Adding Sim Src : $sim_src"
    add_files -fileset $sim_fileset -norecurse $sim_src
}

foreach inc_src $inc_srcs {
    puts "BUILDER: Adding Inc Src : $inc_src"
    add_files -fileset $sim_fileset -norecurse $inc_src
}

# Simulator independent config
set_property top $sim_top [get_filesets $sim_fileset]
set_property default_lib xil_defaultlib [current_project]
update_compile_order -fileset sim_1 -quiet

# Select the simulator
# WARNING: Do this first before setting simulator specific properties!
set_property target_simulator $simulator [current_project]

# Vivado quirk when passing options to external simulators
if [expr [string equal $simulator "XSim"] == 1] {
    append verilog_defs " WORKING_DIR=\"$working_dir\""
} else {
    append verilog_defs " WORKING_DIR=$working_dir"
}

# Pass Verilog definitions to simulation for all files
set_property verilog_define $verilog_defs [get_filesets $sim_fileset]

# XSim specific settings
set_property xsim.simulate.runtime "${sim_runtime}us" -objects [get_filesets $sim_fileset]
set_property xsim.elaborate.debug_level "all" -objects [get_filesets $sim_fileset]
set_property xsim.elaborate.unifast $sim_fast -objects [get_filesets $sim_fileset]
# Set default timescale to prevent bogus warnings
set_property xsim.elaborate.xelab.more_options -value {-timescale 1ns/1ns} -objects [get_filesets $sim_fileset]

# Modelsim specific settings
if [expr [string equal $simulator "Modelsim"] == 1] {
    set sim_64bit       $::env(VIV_SIM_64BIT)

    set_property compxlib.modelsim_compiled_library_dir $sim_complibdir [current_project]
    # Does not work yet (as of Vivado 2015.2), but will be useful for 32-bit support
    # See: http://www.xilinx.com/support/answers/62210.html
    set_property modelsim.64bit $sim_64bit -objects [get_filesets $sim_fileset]
    set_property modelsim.simulate.runtime "${sim_runtime}ns" -objects [get_filesets $sim_fileset]
    set_property modelsim.elaborate.acc "true" -objects [get_filesets $sim_fileset]
    set_property modelsim.simulate.log_all_signals "true" -objects [get_filesets $sim_fileset]
    set_property modelsim.simulate.vsim.more_options -value "-c" -objects [get_filesets $sim_fileset]
    set_property modelsim.elaborate.unifast $sim_fast -objects [get_filesets $sim_fileset]
    if [info exists ::env(VIV_SIM_USER_DO) ] {
        set_property modelsim.simulate.custom_udo -value "$::env(VIV_SIM_USER_DO)" -objects [get_filesets $sim_fileset]
    }
}

# Launch simulation
launch_simulation

if { [info exists ::env(VIV_SYNTH_TOP)] } {
   puts "BUILDER: Synthesizing"
   # Synthesize requested modules
   foreach synth_top "$::env(VIV_SYNTH_TOP)" {
      set_property top $synth_top [current_fileset]
      synth_design -mode out_of_context
      # Perform a simple regex-based search for all clock signals and constrain
      # them to 500 MHz for the timing report.
      set clk_regexp "(?i)^(?!.*en.*).*(clk|clock).*"
      foreach clk_inst [get_ports -regexp $clk_regexp] {
         create_clock -name $clk_inst -period 2.0 [get_ports $clk_inst]
      }
      report_utilization -no_primitives -file ${working_dir}/${synth_top}_synth.rpt
      report_timing_summary -setup -max_paths 3 -unique_pins -no_header -append -file ${working_dir}/${synth_top}_synth.rpt
      write_checkpoint -force ${working_dir}/${synth_top}_synth.dcp
   }
} else {
   puts "BUILDER: Skipping resource report because VIV_SYNTH_TOP is not set"
}
# Close project
if [string equal $vivado_mode "batch"] {
    puts "BUILDER: Closing project"
    close_project
} else {
    puts "BUILDER: In GUI mode. Leaving project open."
}

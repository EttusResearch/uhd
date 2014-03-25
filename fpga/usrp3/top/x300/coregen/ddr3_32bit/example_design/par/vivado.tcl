#
# This is a common pa.tcl used by all designs used for hardware validation
#

# To remove all report and implementation files (if exist any on previous runs)
source rem_files.tcl

# Setup default values
set DEBUG_FILES 1

# Might not need in the future#
create_project proj1 . -force;
set_property design_mode RTL [get_property srcset [current_run]] ;# Set project type to RTL (as apposed to Netlist)

# Define some helpful variables, dirs, etc.
# All defines related to the design should be located in the app directory

set design example_top
set device xc7k410t-2-ffg900
#set constraints_file example_top.ucf




# Setup design sources and constraints
set readfile [open ../synth/example_top.prj r]
while {[gets $readfile line] >=0} {
  set file_list [split $line " "]
  if {[lindex $file_list 0] == "verilog"} {
    read_verilog [lindex $file_list 2]
  } else {
    read_vhdl [lindex $file_list 2]
  }
}

# Read the xdc constraints for the top-level file
set_property top ${design} [get_property srcset [current_run]]
import_files -fileset [get_filesets constrs_1] -force -norecurse {./example_top.xdc}

# Synthesize the design and create a post synthesis data check point
# Also create a port synthesis verilog netlist module
synth_design -part ${device} -top example_top >> synthesis.log;
write_checkpoint post_synth.dcp;
write_verilog ./${design}.v;

# Optimize the design and create a post optimization data check point
opt_design >> optimization.log;
write_checkpoint post_optimize.dcp;

# Place the design and create a post placement data check point
place_design >> place.log;
write_checkpoint post_place.dcp;

# Route the design and create a post routing data check point
route_design >> route.log;
write_checkpoint post_route.dcp;

# Create a static timing analysis report
report_timing -delay_type min_max -sort_by group -max_paths 3 -input_pins -file ${design}.sta;

# Mandatory outputs
report_drc;

if {[report_drc -file example_top_drc.txt -rules {ADEF}]} {
  puts "ERROR -- report_drc bad return status -- should only find warnings"
  exit 1
}

write_bitstream -file ${design}.bit >> bitgen.log;

# Optional 'debug' outputs
if {$DEBUG_FILES} {
    write_edf ./${design}.edf;
    write_verilog ./${design}.routed.v;
    write_xdc ./${design}.planahead.xdc;
    write_ncd -file ${design}.ncd
    write_pcf -file ${design}.pcf
}

close_project -delete;

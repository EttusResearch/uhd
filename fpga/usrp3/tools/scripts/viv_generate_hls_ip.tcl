#
# Copyright 2015 Ettus Research
#

# ---------------------------------------
# Gather all external parameters
# ---------------------------------------
set part_name        $::env(PART_NAME)              ;# Full Xilinx part name
set hls_ip_name      $::env(HLS_IP_NAME)            ;# High level synthesis IP name
set hls_ip_srcs      $::env(HLS_IP_SRCS)            ;# High level synthesis IP source files
set hls_ip_inc       $::env(HLS_IP_INCLUDES)        ;# High level synthesis IP include directories

# ---------------------------------------
# Vivado Commands
# ---------------------------------------
open_project $hls_ip_name
open_solution "solution"
set_part $part_name
set_top $hls_ip_name
puts "BUILDER: Using include location : $hls_ip_inc"
foreach src_file $hls_ip_srcs {
    set src_ext [file extension $src_file ]
    if [expr [lsearch {.c .cpp .cc .h .hpp} $src_ext] >= 0] {
        puts "BUILDER: Adding C/C++ : $src_file"
        add_files $src_file -cflags "-I $hls_ip_inc"
    } elseif [expr [lsearch {.tcl} $src_ext] >= 0] {
        puts "BUILDER: Executing tcl script : $src_file"
        source $src_file
    } else {
        puts "BUILDER: \[WARNING\] File ignored!!!: $src_file"
    }
}
csynth_design
export_design -format ip_catalog

exit

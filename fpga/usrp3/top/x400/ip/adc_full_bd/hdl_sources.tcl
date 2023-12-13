set script_loc [file normalize [info script]]
set script_dir [file dirname $script_loc]

# Vivado's block diagram default library for files not belonging to any special
# library is called xil_defaultlib
read_verilog -library xil_defaultlib $script_dir/../../rf/common/adc_iq_repacker.v

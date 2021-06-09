set script_loc [file normalize [info script]]
set script_dir [file dirname $script_loc]

# Vivado's block diagram default library for files not belonging to any special
# library is called xil_defaultlib
read_verilog -library xil_defaultlib $script_dir/../../rf/400m/adc_gearbox_8x4.v
read_vhdl    -library xil_defaultlib $script_dir/../../rf/common/PkgRf.vhd
read_vhdl    -library xil_defaultlib $script_dir/../../rf/400m/adc_gearbox_2x4.vhd
read_vhdl    -library xil_defaultlib $script_dir/../../rf/400m/ddc_400m_saturate.vhd
read_vhdl    -library xil_defaultlib $script_dir/../../rf/common/scale_2x.vhd

set script_loc [file normalize [info script]]
set script_dir [file dirname $script_loc]

read_verilog -library work $script_dir/../../rf/400m/dac_gearbox_4x2.v
read_vhdl    -library work $script_dir/../../rf/common/PkgRf.vhd
read_vhdl    -library work $script_dir/../../rf/400m/dac_gearbox_6x8.vhd
read_vhdl    -library work $script_dir/../../rf/400m/dac_gearbox_6x12.vhd
read_vhdl    -library work $script_dir/../../rf/400m/dac_gearbox_12x8.vhd
read_vhdl    -library work $script_dir/../../rf/400m/duc_400m_saturate.vhd

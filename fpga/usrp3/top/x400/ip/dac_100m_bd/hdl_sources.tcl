set script_loc [file normalize [info script]]
set script_dir [file dirname $script_loc]

read_vhdl    -library work $script_dir/../../rf/100m/duc_saturate.vhd
read_vhdl    -library work $script_dir/../../rf/100m/dac_1_3_clk_converter.vhd
read_vhdl    -library work $script_dir/../../rf/100m/dac_2_1_clk_converter.vhd

set script_loc [file normalize [info script]]
set script_dir [file dirname $script_loc]

read_vhdl           -library work $script_dir/../../rf/100m/adc_3_1_clk_converter.vhd
read_verilog        -library work $script_dir/../../rf/100m/adc_gearbox_8x1.v
read_vhdl           -library work $script_dir/../../rf/100m/ddc_saturate.vhd
read_vhdl           -library work $script_dir/../../rf/common/PkgRf.vhd
read_vhdl           -library work $script_dir/../../rf/common/scale_2x.vhd

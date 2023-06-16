set script_loc [file normalize [info script]]
set script_dir [file dirname $script_loc]

read_verilog -library work $script_dir/../../../rf/common/capture_sysref.v
read_verilog -library work $script_dir/../../../../../lib/control/synchronizer.v
read_verilog -library work $script_dir/../../../../../lib/control/synchronizer_impl.v
read_verilog -library work $script_dir/../../../rf/common/sync_wrapper.v
read_vhdl    -library work $script_dir/../../../rf/common/rf_nco_reset.vhd
read_vhdl    -library work $script_dir/../../../regmap/x410/PkgRFDC_REGS_REGMAP.vhd
read_vhdl    -library work $script_dir/../../../rf/x410/x410_rf_reset_controller.vhd
read_vhdl    -library work $script_dir/../../../rf/common/rf_reset.vhd
read_vhdl    -library work $script_dir/../../../rf/x410/x410_clock_gates.vhd
read_vhdl    -library work $script_dir/../../../rf/common/axis_mux.vhd
read_vhdl    -library work $script_dir/../../../rf/common/gpio_to_axis_mux.vhd

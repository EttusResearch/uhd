set script_loc [file normalize [info script]]
set script_dir [file dirname $script_loc]

read_verilog        -library work $script_dir/../../../../../lib/axi/axi_defs.v
read_verilog        -library work $script_dir/../../../../../lib/control/axil_ctrlport_master.v
read_verilog        -library work $script_dir/../../../../../lib/control/pulse_synchronizer.v
read_verilog        -library work $script_dir/../../../../../lib/control/reset_sync.v
read_verilog        -library work $script_dir/../../../../../lib/control/synchronizer.v
read_verilog        -library work $script_dir/../../../../../lib/control/synchronizer_impl.v
read_verilog        -library work $script_dir/../../../../../lib/rfnoc/core/ctrlport.vh
read_vhdl           -library work $script_dir/../../../regmap/x440/PkgRFDC_REGS_REGMAP.vhd
read_verilog        -library work $script_dir/../../../regmap/x440/rfdc_mapping_regmap_utils.vh
read_verilog        -library work $script_dir/../../../rf/common/axi_rfdc_info_memory.v
read_vhdl           -library work $script_dir/../../../rf/common/axis_mux.vhd
read_verilog        -library work $script_dir/../../../rf/common/capture_sysref.v
read_vhdl           -library work $script_dir/../../../rf/common/gpio_to_axis_mux.vhd
read_verilog -sv    -library work $script_dir/../../../rf/common/rf_nco_reset_controller.sv
read_verilog        -library work $script_dir/../../../rf/common/rf_nco_reset_wrapper.v
read_vhdl           -library work $script_dir/../../../rf/common/rf_reset.vhd
read_verilog -sv    -library work $script_dir/../../../rf/common/rfdc_info_memory.sv
read_verilog -sv    -library work $script_dir/../../../rf/common/rfdc_info_pkg.sv
read_verilog        -library work $script_dir/../../../rf/common/sync_wrapper.v
read_vhdl           -library work $script_dir/../../../rf/x440/x440_clock_gates.vhd
read_vhdl           -library work $script_dir/../../../rf/x440/x440_rf_reset_controller.vhd
read_verilog -sv    -library work $script_dir/../../../rf/x440/x440_rfdc_memory_content_pkg.sv
read_verilog        -library work $script_dir/../../../rf/x440/x440_rfdc_tx_control_remap.v

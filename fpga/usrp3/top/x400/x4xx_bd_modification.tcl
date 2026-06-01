
# Extract the Verilog defines from the environment (optional variable)
set viv_defs ""
if { [info exists ::env(VIV_VERILOG_DEFS)] } {
    set viv_defs $::env(VIV_VERILOG_DEFS)
}

# Only apply these modifications for X440 with 200M RF bandwidth image
if {[regexp {X440=1} $viv_defs] && [regexp {RF_BW=200} $viv_defs] && [regexp {USE_FABRIC_RESAMPLERS=1} $viv_defs]} {

  # print a message to indicate that the modifications are being applied
  puts "Applying X440 200M RF modifications to block design"

  # open the existing block design
  open_bd_design [get_files x440_ps_rfdc_bd.bd]

  # Change the RFDC data path to 2 samples / cycle.
  # This requires the decimation and interpolation modes to be set to 8.
  # As the original configuration is 8 samples / cycle at using 2 times
  # resampling the clock rates stay the same.
  set_property -dict [list \
    CONFIG.ADC0_Multi_Tile_Sync {true} \
    CONFIG.ADC_Decimation_Mode00 {8} \
    CONFIG.ADC_Data_Width00 {2} \
    CONFIG.ADC_Decimation_Mode01 {8} \
    CONFIG.ADC_Data_Width01 {2} \
    CONFIG.ADC_Data_Width02 {2} \
    CONFIG.ADC_Data_Width03 {2} \
    CONFIG.ADC1_Multi_Tile_Sync {true} \
    CONFIG.ADC_Data_Width10 {2} \
    CONFIG.ADC_Data_Width11 {2} \
    CONFIG.ADC_Data_Width12 {2} \
    CONFIG.ADC_Data_Width13 {2} \
    CONFIG.ADC2_Multi_Tile_Sync {true} \
    CONFIG.ADC_Data_Width20 {2} \
    CONFIG.ADC_Data_Width21 {2} \
    CONFIG.ADC_Data_Width22 {2} \
    CONFIG.ADC_Data_Width23 {2} \
    CONFIG.ADC3_Multi_Tile_Sync {true} \
    CONFIG.ADC_Data_Width30 {2} \
    CONFIG.ADC_Data_Width31 {2} \
    CONFIG.ADC_Data_Width32 {2} \
    CONFIG.ADC_Data_Width33 {2} \
    CONFIG.DAC0_Multi_Tile_Sync {true} \
    CONFIG.DAC_Data_Width00 {4} \
    CONFIG.DAC_Data_Width01 {4} \
    CONFIG.DAC_Data_Width02 {4} \
    CONFIG.DAC_Data_Width03 {4} \
    CONFIG.DAC1_Multi_Tile_Sync {true} \
    CONFIG.DAC_Data_Width10 {4} \
    CONFIG.DAC_Data_Width11 {4} \
    CONFIG.DAC_Data_Width12 {4} \
    CONFIG.DAC_Data_Width13 {4} \
    ] [get_bd_cells rfdc/rf_data_converter]


  # The changed RFDC interfaces have to be reflected in the upstream AXI
  # interfaces. A change to 2 samples/cycle is required for the calibration
  # muxes.
  # As the file ordering is not yet done at this point in the flow. This means
  # that the calibration mux kAxiWidth property might not be applied. This leads
  # to critical warnings during synthesis about missing width of the AXI
  # interfaces. As we narrow the source and desination with the calibration
  # muxes in between the functionality is not affected as we just use the LSBs.
  # The unused bits from the wider original 256 bit wide interfaces are
  # optimized away during synthesis.
  set_property -dict [list CONFIG.kAxiWidth {64}] [get_bd_cells rfdc/calibration_muxes/gpio_to_axis_mux_0]
  set_property -dict [list CONFIG.TDATA_NUM_BYTES {8}] [get_bd_intf_ports dac_tile228_ch0_din]
  set_property -dict [list CONFIG.TDATA_NUM_BYTES {8}] [get_bd_intf_ports dac_tile228_ch1_din]
  set_property -dict [list CONFIG.TDATA_NUM_BYTES {8}] [get_bd_intf_ports dac_tile228_ch2_din]
  set_property -dict [list CONFIG.TDATA_NUM_BYTES {8}] [get_bd_intf_ports dac_tile228_ch3_din]
  set_property -dict [list CONFIG.TDATA_NUM_BYTES {8}] [get_bd_intf_ports dac_tile229_ch0_din]
  set_property -dict [list CONFIG.TDATA_NUM_BYTES {8}] [get_bd_intf_ports dac_tile229_ch1_din]
  set_property -dict [list CONFIG.TDATA_NUM_BYTES {8}] [get_bd_intf_ports dac_tile229_ch2_din]
  set_property -dict [list CONFIG.TDATA_NUM_BYTES {8}] [get_bd_intf_ports dac_tile229_ch3_din]

  # revalidate and save the block design
  validate_bd_design
  save_bd_design

  # regenerate the output products to avoid any stale intermediate files
  generate_target all [get_files x440_ps_rfdc_bd.bd]

  # close the block design after applying all changes
  close_bd_design [current_bd_design]
}

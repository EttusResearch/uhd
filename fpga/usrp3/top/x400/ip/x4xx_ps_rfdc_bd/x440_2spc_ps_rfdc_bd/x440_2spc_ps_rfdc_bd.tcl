#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Module: x440_2spc_ps_rfdc_bd
#
# Description:
#
#   Manual X440 RFDC block-design wrapper for the 2-samples-per-cycle
#   configuration. The base block-design Tcl is generated from the original
#   X440 design with its design and clock-domain names adapted for this
#   variant. This script applies the RFDC data-width and interface-width
#   changes required by the 2-SPC datapath, then validates and saves the
#   resulting block design.
#

# Reuse the original x440 block diagram after renaming.
source ../x440_2spc_ps_rfdc_bd_base/x440_2spc_ps_rfdc_bd_base.tcl

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

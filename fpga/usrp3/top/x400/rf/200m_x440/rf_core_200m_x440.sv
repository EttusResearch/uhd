//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rf_core_200m_x440
//
// Description:
//
//   RF core for X440 daughterboard including data reordering for the RFDC and
//   fixed resamplers by 3 (decimation on RX and interpolation on TX).
//
//   Data/RF Specs:
//     DBs:   1
//     Data Rate RFDC: rfdc_clk @ RADIO_SPC
//     Data Rate RFNoC: data_clk @ RADIO_SPC
//

module rf_core_200m_x440
  import ctrlport_pkg::*;
# (
  parameter int NUM_ADC_CHANNELS = 1,
  parameter int NUM_DAC_CHANNELS = 1,
  localparam int RADIO_SPC = 2
) (
  //---------------------------------------------------------------------------
  // Clocking
  //---------------------------------------------------------------------------

  // Main Clock Inputs
  input  logic data_clk,
  input  logic data_clk_2x,
  input  logic rfdc_clk,
  input  logic pll_ref_clk,

  // Resets
  input  logic rx_resampler_reset_pulse_dclk,
  input  logic tx_resampler_reset_pulse_dclk,

  // AXI4-Lite Config Clock
  // This clock is used to synchronize status bits for the RFDC
  // registers in the AXI-S clock domain.
  input  logic s_axi_config_clk,

  //---------------------------------------------------------------------------
  // RFDC Data Interfaces (rfdc_clk domain)
  //---------------------------------------------------------------------------
  // ADC
  input  logic [16*RADIO_SPC-1:0]     adc_data_in_i_tdata [0:NUM_ADC_CHANNELS-1],
  output logic [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tready,
  input  logic [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tvalid,
  input  logic [16*RADIO_SPC-1:0]     adc_data_in_q_tdata [0:NUM_ADC_CHANNELS-1],
  output logic [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tready,
  input  logic [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tvalid,

  // DAC
  output logic [32*RADIO_SPC-1:0]     dac_data_out_tdata [0:NUM_DAC_CHANNELS-1],
  input  logic [NUM_DAC_CHANNELS-1:0] dac_data_out_tready,
  output logic [NUM_DAC_CHANNELS-1:0] dac_data_out_tvalid,

  //---------------------------------------------------------------------------
  // User Data Interfaces (data_clk domain)
  //---------------------------------------------------------------------------
  // ADC
  // Packed [Q1,I1,Q0,I0] with Q in MSBs
  output logic [32*RADIO_SPC-1:0]     adc_data_out_tdata [0:NUM_ADC_CHANNELS-1],
  output logic [NUM_ADC_CHANNELS-1:0] adc_data_out_tvalid,

  // DAC
  input  logic [32*RADIO_SPC-1:0]     dac_data_in_tdata [0:NUM_DAC_CHANNELS-1],
  output logic [NUM_DAC_CHANNELS-1:0] dac_data_in_tready,
  input  logic [NUM_DAC_CHANNELS-1:0] dac_data_in_tvalid,

  //---------------------------------------------------------------------------
  // Miscellaneous
  //---------------------------------------------------------------------------
  // Invert I/Q control signals from RFDC to DSP chain.
  input  logic [NUM_ADC_CHANNELS-1:0] invert_adc_iq_rclk,
  input  logic [NUM_DAC_CHANNELS-1:0] invert_dac_iq_rclk,

  // Control/status vectors from/to RFDC.
  // Notice these are all in the s_axi_config_clk domain.
  output logic [ 9:0] dsp_info_sclk,
  output logic [15:0] axi_status_sclk,
  output logic [15:0] rfdc_info_sclk,

  // Enable signal
  input logic adc_enable_data_rclk,
  input logic adc_rfdc_axi_resetn_rclk,

  // Version (Constant)
  output logic [95:0] version_info
);

  `include "../../regmap/x440/rfdc_regs_regmap_utils.vh"
  `include "../../regmap/x440/versioning_regs_regmap_utils.vh"
  `include "../../regmap/versioning_utils.vh"

  //---------------------------------------------------------------------------
  // RFDC conversion
  //---------------------------------------------------------------------------
  // ADC
  // Packed [Q1,I1,Q0,I0] with Q in MSBs
  logic [32*RADIO_SPC-1:0]     adc_data_int_tdata [0:NUM_ADC_CHANNELS-1];
  logic [NUM_ADC_CHANNELS-1:0] adc_data_int_tvalid;

  // DAC
  logic [32*RADIO_SPC-1:0]     dac_data_int_tdata [0:NUM_DAC_CHANNELS-1];
  logic [NUM_DAC_CHANNELS-1:0] dac_data_int_tvalid;

  // DSP information
  logic [15:0] rfdc_info_int;

  rf_core_full #(
    .NUM_ADC_CHANNELS(NUM_ADC_CHANNELS),
    .NUM_DAC_CHANNELS(NUM_DAC_CHANNELS),
    .RADIO_SPC(RADIO_SPC)
  ) rf_core_full_i (
    .rfdc_clk                 (rfdc_clk),
    .s_axi_config_clk         (s_axi_config_clk),
    .adc_data_in_i_tdata      (adc_data_in_i_tdata),
    .adc_data_in_i_tready     (adc_data_in_i_tready),
    .adc_data_in_i_tvalid     (adc_data_in_i_tvalid),
    .adc_data_in_q_tdata      (adc_data_in_q_tdata),
    .adc_data_in_q_tready     (adc_data_in_q_tready),
    .adc_data_in_q_tvalid     (adc_data_in_q_tvalid),
    .dac_data_out_tdata       (dac_data_out_tdata),
    .dac_data_out_tready      (dac_data_out_tready),
    .dac_data_out_tvalid      (dac_data_out_tvalid),
    .adc_data_out_tdata       (adc_data_int_tdata),
    .adc_data_out_tvalid      (adc_data_int_tvalid),
    .dac_data_in_tdata        (dac_data_int_tdata),
    .dac_data_in_tready       (),
    .dac_data_in_tvalid       (dac_data_int_tvalid),
    .invert_adc_iq_rclk       (invert_adc_iq_rclk),
    .invert_dac_iq_rclk       (invert_dac_iq_rclk),
    .dsp_info_sclk            (dsp_info_sclk),
    .rfdc_info_sclk           (rfdc_info_int),
    .axi_status_sclk          (axi_status_sclk),
    .adc_enable_data_rclk     (adc_enable_data_rclk),
    .adc_rfdc_axi_resetn_rclk (adc_rfdc_axi_resetn_rclk),
    .version_info             ()
  );

  //---------------------------------------------------------------------------
  // Fixed resamplers by 3
  //---------------------------------------------------------------------------
  for (genvar i = 0; i < NUM_DAC_CHANNELS; i++) begin
    tx_inp3 tx_inp3_i (
      .data_clk              (data_clk),
      .data_clk_2x           (data_clk_2x),
      .rfdc_clk              (rfdc_clk),
      .pll_ref_clk           (pll_ref_clk),
      .reset_pulse_dclk      (tx_resampler_reset_pulse_dclk),
      .dac_data_in_tdata     (dac_data_in_tdata[i]),
      .dac_data_in_tvalid    (dac_data_in_tvalid[i]),
      .dac_data_out_tdata    (dac_data_int_tdata[i]),
      .dac_data_out_tvalid   (dac_data_int_tvalid[i])
    );
    assign dac_data_in_tready[i] = '1;
  end

  for (genvar d = 0; d < NUM_ADC_CHANNELS; d++) begin
    rx_dec3 rx_dec3_i (
      .rfdc_clk              (rfdc_clk),
      .data_clk              (data_clk),
      .pll_ref_clk           (pll_ref_clk),
      .reset_pulse_dclk      (rx_resampler_reset_pulse_dclk),
      .adc_data_in_tdata     (adc_data_int_tdata[d]),
      .adc_data_in_tvalid    (adc_data_int_tvalid[d]),
      .adc_data_in_tready    (),
      .adc_data_out_tdata    (adc_data_out_tdata[d]),
      .adc_data_out_tvalid   (adc_data_out_tvalid[d])
    );
  end


  //---------------------------------------------------------------------------
  // Versioning
  //---------------------------------------------------------------------------
  // add information about resamplers
  always_comb begin
    rfdc_info_sclk = rfdc_info_int;
    rfdc_info_sclk[RFDC_INFO_XTRA_RESAMP_MSB:RFDC_INFO_XTRA_RESAMP] = 'd3;
  end

  // check version info from rf_core_full
  if (build_version(
      RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_MAJOR,
      RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_MINOR,
      RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_BUILD
    ) != build_version(1,0,0)) begin
    $error("rf_core_200m_x440: Incompatible rf_core_full version detected!");
  end

  // assign version info
  assign version_info = build_component_versions(
    RF_CORE_200M_VERSION_LAST_MODIFIED_TIME,
    build_version(
      RF_CORE_200M_OLDEST_COMPATIBLE_VERSION_MAJOR,
      RF_CORE_200M_OLDEST_COMPATIBLE_VERSION_MINOR,
      RF_CORE_200M_OLDEST_COMPATIBLE_VERSION_BUILD
    ),
    build_version(
      RF_CORE_200M_CURRENT_VERSION_MAJOR,
      RF_CORE_200M_CURRENT_VERSION_MINOR,
      RF_CORE_200M_CURRENT_VERSION_BUILD
    )
  );

endmodule

//XmlParse xml_on
//<regmap name="VERSIONING_REGS_REGMAP">
//  <group name="VERSIONING_CONSTANTS">
//    <enumeratedtype name="RF_CORE_200M_VERSION" showhex="true">
//      <info>
//        200 MHz RF core (similar to full RF core but with fixed resamplers by 3).{BR/}
//        For guidance on when to update these revision numbers,
//        please refer to the register map documentation accordingly:
//        <li> Current version: @.VERSIONING_REGS_REGMAP..CURRENT_VERSION
//        <li> Oldest compatible version: @.VERSIONING_REGS_REGMAP..OLDEST_COMPATIBLE_VERSION
//        <li> Version last modified: @.VERSIONING_REGS_REGMAP..VERSION_LAST_MODIFIED
//      </info>
//      <value name="RF_CORE_200M_CURRENT_VERSION_MAJOR"           integer="1"/>
//      <value name="RF_CORE_200M_CURRENT_VERSION_MINOR"           integer="0"/>
//      <value name="RF_CORE_200M_CURRENT_VERSION_BUILD"           integer="0"/>
//      <value name="RF_CORE_200M_OLDEST_COMPATIBLE_VERSION_MAJOR" integer="1"/>
//      <value name="RF_CORE_200M_OLDEST_COMPATIBLE_VERSION_MINOR" integer="0"/>
//      <value name="RF_CORE_200M_OLDEST_COMPATIBLE_VERSION_BUILD" integer="0"/>
//      <value name="RF_CORE_200M_VERSION_LAST_MODIFIED_TIME"      integer="0x26052017"/>
//    </enumeratedtype>
//  </group>
//</regmap>
//XmlParse xml_off

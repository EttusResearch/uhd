//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rf_core_full
//
// Description:
//
//   Top-level wrapper for the ADC/DAC processing logic. One of these wrappers
//   exists for every supported Data Rate. An instance of this core should
//   exist per dboard.
//
//   Data/RF Specs:
//     DBs:   1
//     RX/DB: via parameter NUM_ADC_CHANNELS
//     TX/DB: via parameter NUM_DAC_CHANNELS
//     Data Rate: rfdc_clk @ RADIO_SPC
//

`default_nettype none

module rf_core_full # (
  parameter NUM_ADC_CHANNELS = 4,
  parameter NUM_DAC_CHANNELS = 4,
  parameter RADIO_SPC = 8
) (

  //---------------------------------------------------------------------------
  // Clocking
  //---------------------------------------------------------------------------

  // Main Clock Inputs
  input  wire rfdc_clk,

  // AXI4-Lite Config Clock
  // This clock is used to synchronize status bits for the RFDC
  // registers in the AXI-S clock domain.
  input  wire s_axi_config_clk,

  //---------------------------------------------------------------------------
  // RFDC Data Interfaces
  //---------------------------------------------------------------------------
  // All ports here are in the rfdc_clk domain.

  // ADC
  input  wire [16*RADIO_SPC-1:0]     adc_data_in_i_tdata [0:NUM_ADC_CHANNELS-1],
  output wire [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tready,
  input  wire [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tvalid,
  input  wire [16*RADIO_SPC-1:0]     adc_data_in_q_tdata [0:NUM_ADC_CHANNELS-1],
  output wire [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tready,
  input  wire [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tvalid,

  // DAC
  output wire [32*RADIO_SPC-1:0]     dac_data_out_tdata [0:NUM_DAC_CHANNELS-1],
  input  wire [NUM_DAC_CHANNELS-1:0] dac_data_out_tready,
  output wire [NUM_DAC_CHANNELS-1:0] dac_data_out_tvalid,

  //---------------------------------------------------------------------------
  // User Data Interfaces
  //---------------------------------------------------------------------------
  // All ports here are in the rfdc_clk domain on the X440.

  // ADC
  // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  output wire [32*RADIO_SPC-1:0]     adc_data_out_tdata [0:NUM_ADC_CHANNELS-1],
  output wire [NUM_ADC_CHANNELS-1:0] adc_data_out_tvalid,

  // DAC
  input  wire [32*RADIO_SPC-1:0]     dac_data_in_tdata [0:NUM_DAC_CHANNELS-1],
  output wire [NUM_DAC_CHANNELS-1:0] dac_data_in_tready,
  input  wire [NUM_DAC_CHANNELS-1:0] dac_data_in_tvalid,

  //---------------------------------------------------------------------------
  // Miscellaneous
  //---------------------------------------------------------------------------

  // Invert I/Q control signals from RFDC to DSP chain.
  input  wire [NUM_ADC_CHANNELS-1:0] invert_adc_iq_rclk,
  input  wire [NUM_DAC_CHANNELS-1:0] invert_dac_iq_rclk,

  // Control/status vectors from/to RFDC.
  // Notice these are all in the s_axi_config_clk domain.
  output wire [ 9:0] dsp_info_sclk,
  output wire [15:0] axi_status_sclk,
  output wire [15:0] rfdc_info_sclk,

  // Resets.
  input wire adc_enable_data_rclk,
  input wire adc_rfdc_axi_resetn_rclk,

  // Version (Constant)
  output wire [95:0] version_info
);

  `include "../../regmap/x440/rfdc_regs_regmap_utils.vh"
  `include "../../regmap/x440/versioning_regs_regmap_utils.vh"
  `include "../../regmap/versioning_utils.vh"

  //---------------------------------------------------------------------------
  // Resets, Debug and Misc.
  //---------------------------------------------------------------------------
  wire [15:0] axi_status;

  // Group all these status bits together. They don't toggle frequently so data
  // coherency is not an issue here.
  // Using constants for DB0 since the bits are the 16 LSBs in a 32-bit vector.
  // DB1 simply uses the 16 MSBs when wiring the status vector.
  // There is no tready going to the ADC (one has to be always ready for ADC
  // data), but it is still a component of the axi_status vector as a generic
  // AXI stream status. Report 1'b1 to the status vector consistent with being
  // always ready
  assign axi_status[USER_ADC_TREADY_MSB  :USER_ADC_TREADY  ] = '1;
  assign axi_status[USER_ADC_TVALID_MSB  :USER_ADC_TVALID  ] = adc_data_out_tvalid;
  assign axi_status[RFDC_ADC_I_TVALID_MSB:RFDC_ADC_I_TVALID] = adc_data_in_i_tvalid;
  assign axi_status[RFDC_ADC_Q_TVALID_MSB:RFDC_ADC_Q_TVALID] = adc_data_in_q_tvalid;
  assign axi_status[RFDC_ADC_I_TREADY_MSB:RFDC_ADC_I_TREADY] = adc_data_in_i_tready;
  assign axi_status[RFDC_ADC_Q_TREADY_MSB:RFDC_ADC_Q_TREADY] = adc_data_in_q_tready;
  assign axi_status[RFDC_DAC_TVALID_MSB  :RFDC_DAC_TVALID  ] = dac_data_out_tvalid;
  assign axi_status[RFDC_DAC_TREADY_MSB  :RFDC_DAC_TREADY  ] = dac_data_out_tready;

  synchronizer #(
    .WIDTH             (16),
    .STAGES            (2),
    .INITIAL_VAL       (0),
    .FALSE_PATH_TO_IN  (1)
  ) synchronizer_axis_status (
    .clk  (s_axi_config_clk),
    .rst  (1'b0),
    .in   (axi_status),
    .out  (axi_status_sclk)
  );

  // Drive the DSP info vector with information on this specific DSP chain.
  assign dsp_info_sclk[FABRIC_DSP_RX_CNT_MSB:FABRIC_DSP_RX_CNT] = NUM_ADC_CHANNELS;
  assign dsp_info_sclk[FABRIC_DSP_TX_CNT_MSB:FABRIC_DSP_TX_CNT] = NUM_DAC_CHANNELS;

  // This RF core is designed to route through the RFDC using the same SPC setting.
  // The SPC setting is given as a parameter to this module.
  assign rfdc_info_sclk[RFDC_INFO_SPC_RX_MSB:RFDC_INFO_SPC_RX] = $clog2(RADIO_SPC);
  assign rfdc_info_sclk[RFDC_INFO_SPC_TX_MSB:RFDC_INFO_SPC_TX] = $clog2(RADIO_SPC*2);
  // This RF core module contains no additional resampling
  assign rfdc_info_sclk[RFDC_INFO_XTRA_RESAMP_MSB:RFDC_INFO_XTRA_RESAMP] = 4'd1;

  //---------------------------------------------------------------------------
  // ADC Post-Processing
  //---------------------------------------------------------------------------

  // There is no tready going to the ADC (one has to be always ready for ADC
  // data), but it is still a component of the axi_status vector as a generic
  // AXI stream status. Report 1'b1 to the status vector consistent with being
  // always ready.
  assign adc_data_in_i_tready = '1;
  assign adc_data_in_q_tready = '1;

  // ADC Data from the RFDC arrives here with separate I and Q
  // streams. It gets combined to a single stream.
  for (genvar adc_num = 0; adc_num < (NUM_ADC_CHANNELS); adc_num++)
  begin : adc_gen
    //signals between packer and register
    wire [32*RADIO_SPC-1:0] packer_to_reg_tdata;
    wire                    packer_to_reg_tvalid;

    adc_iq_repacker #(
      .SPC           (RADIO_SPC),
      .SAMPLE_WIDTH  (16)
    ) iq_repacker (
      .clk             (rfdc_clk),
      .adc_q_in        (adc_data_in_q_tdata[adc_num]),
      .adc_i_in        (adc_data_in_i_tdata[adc_num]),
      .valid_in        (adc_data_in_i_tvalid[adc_num]), // taking I stream valid bit only to reduce complexity
      .enable          (adc_enable_data_rclk),
      .swap_iq         (invert_adc_iq_rclk[adc_num]),
      .data_out_tdata  (packer_to_reg_tdata),
      .data_out_tvalid (packer_to_reg_tvalid)
    );

    // Create instances of axi_fifo_flop2 module for each ADC channel
    axi_fifo_flop2 #(
      .WIDTH (32*RADIO_SPC)
    ) adc_reg (
      .clk (rfdc_clk),
      .reset ('0),
      .clear ('0),
      .i_tdata (packer_to_reg_tdata),
      .i_tvalid (packer_to_reg_tvalid),
      .i_tready (),
      .o_tdata (adc_data_out_tdata[adc_num]),
      .o_tvalid (adc_data_out_tvalid[adc_num]),
      .o_tready ('1),
      .space (),
      .occupied ()
    );
  end

  //---------------------------------------------------------------------------
  // DAC Pre-Processing
  //---------------------------------------------------------------------------

  for (genvar dac_num = 0; dac_num < NUM_DAC_CHANNELS; dac_num++)
  begin : dac_swap_gen

    for (genvar sample_num = 0; sample_num < RADIO_SPC; sample_num++)
    begin : sample_swap_gen

      assign dac_data_out_tdata[dac_num][32*sample_num+00 +: 16] = invert_dac_iq_rclk[dac_num] ?
        dac_data_in_tdata[dac_num][32*sample_num+16 +: 16] :
        dac_data_in_tdata[dac_num][32*sample_num+00 +: 16];
      assign dac_data_out_tdata[dac_num][32*sample_num+16 +: 16] = invert_dac_iq_rclk[dac_num] ?
        dac_data_in_tdata[dac_num][32*sample_num+00 +: 16] :
        dac_data_in_tdata[dac_num][32*sample_num+16 +: 16];
    end
  end

  assign dac_data_out_tvalid = dac_data_in_tvalid;
  assign dac_data_in_tready  = dac_data_out_tready;

  //---------------------------------------------------------------------------
  // Version
  //---------------------------------------------------------------------------

  // Version metadata, constants come from auto-generated
  // versioning_regs_regmap_utils.vh
  assign version_info = build_component_versions(
    RF_CORE_FULL_VERSION_LAST_MODIFIED_TIME,
    build_version(
      RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_MAJOR,
      RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_MINOR,
      RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_BUILD
    ),
    build_version(
      RF_CORE_FULL_CURRENT_VERSION_MAJOR,
      RF_CORE_FULL_CURRENT_VERSION_MINOR,
      RF_CORE_FULL_CURRENT_VERSION_BUILD
    )
  );

endmodule

`default_nettype wire

//XmlParse xml_on
//<regmap name="VERSIONING_REGS_REGMAP">
//  <group name="VERSIONING_CONSTANTS">
//    <enumeratedtype name="RF_CORE_FULL_VERSION" showhex="true">
//      <info>
//        Full BW RF core.{BR/}
//        For guidance on when to update these revision numbers,
//        please refer to the register map documentation accordingly:
//        <li> Current version: @.VERSIONING_REGS_REGMAP..CURRENT_VERSION
//        <li> Oldest compatible version: @.VERSIONING_REGS_REGMAP..OLDEST_COMPATIBLE_VERSION
//        <li> Version last modified: @.VERSIONING_REGS_REGMAP..VERSION_LAST_MODIFIED
//      </info>
//      <value name="RF_CORE_FULL_CURRENT_VERSION_MAJOR"           integer="1"/>
//      <value name="RF_CORE_FULL_CURRENT_VERSION_MINOR"           integer="1"/>
//      <value name="RF_CORE_FULL_CURRENT_VERSION_BUILD"           integer="0"/>
//      <value name="RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_MAJOR" integer="1"/>
//      <value name="RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_MINOR" integer="0"/>
//      <value name="RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_BUILD" integer="0"/>
//      <value name="RF_CORE_FULL_VERSION_LAST_MODIFIED_TIME"      integer="0x24080915"/>
//    </enumeratedtype>
//  </group>
//</regmap>
//
//<regmap name="RF_CORE_REGMAP" generateverilog="false" generatesv="false" />
//
//XmlParse xml_off

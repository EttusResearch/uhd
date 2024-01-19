//
// Copyright 2023 Ettus Research, a National Instruments Brand
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
//     RX/DB: 4
//     TX/DB: 4
//     Data Rate: rfdc_clk @ 8 SPC
//
//   Input Clocks, all aligned to one another and coming from same MMCM
//     rfdc_clk:    15.625 to 250 MHz
//     rfdc_clk_2x: 2 * rfdc_clk
//

`default_nettype none

module rf_core_full # (
  parameter NUM_ADC_CHANNELS = 4,
  parameter NUM_DAC_CHANNELS = 4
) (

  //---------------------------------------------------------------------------
  // Clocking
  //---------------------------------------------------------------------------

  // Main Clock Inputs
  input  wire rfdc_clk,
  input  wire rfdc_clk_2x,

  // AXI4-Lite Config Clock
  // This clock is used to synchronize status bits for the RFDC
  // registers in the AXI-S clock domain.
  input  wire s_axi_config_clk,


  //---------------------------------------------------------------------------
  // RFDC Data Interfaces
  //---------------------------------------------------------------------------
  // All ports here are in the rfdc_clk domain.

  // ADC
  input  wire [127:0] adc_data_in_i_tdata_0,
  output wire         adc_data_in_i_tready_0,
  input  wire         adc_data_in_i_tvalid_0,
  input  wire [127:0] adc_data_in_q_tdata_0,
  output wire         adc_data_in_q_tready_0,
  input  wire         adc_data_in_q_tvalid_0,
  input  wire [127:0] adc_data_in_i_tdata_1,
  output wire         adc_data_in_i_tready_1,
  input  wire         adc_data_in_i_tvalid_1,
  input  wire [127:0] adc_data_in_q_tdata_1,
  output wire         adc_data_in_q_tready_1,
  input  wire         adc_data_in_q_tvalid_1,
  input  wire [127:0] adc_data_in_i_tdata_2,
  output wire         adc_data_in_i_tready_2,
  input  wire         adc_data_in_i_tvalid_2,
  input  wire [127:0] adc_data_in_q_tdata_2,
  output wire         adc_data_in_q_tready_2,
  input  wire         adc_data_in_q_tvalid_2,
  input  wire [127:0] adc_data_in_i_tdata_3,
  output wire         adc_data_in_i_tready_3,
  input  wire         adc_data_in_i_tvalid_3,
  input  wire [127:0] adc_data_in_q_tdata_3,
  output wire         adc_data_in_q_tready_3,
  input  wire         adc_data_in_q_tvalid_3,


  // DAC
  output wire [255:0] dac_data_out_tdata_0,
  input  wire         dac_data_out_tready_0,
  output wire         dac_data_out_tvalid_0,
  output wire [255:0] dac_data_out_tdata_1,
  input  wire         dac_data_out_tready_1,
  output wire         dac_data_out_tvalid_1,
  output wire [255:0] dac_data_out_tdata_2,
  input  wire         dac_data_out_tready_2,
  output wire         dac_data_out_tvalid_2,
  output wire [255:0] dac_data_out_tdata_3,
  input  wire         dac_data_out_tready_3,
  output wire         dac_data_out_tvalid_3,


  //---------------------------------------------------------------------------
  // User Data Interfaces
  //---------------------------------------------------------------------------
  // All ports here are in the rfdc_clk domain on the X440.

  // ADC
  output wire [255:0] adc_data_out_tdata_0,  // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  output wire         adc_data_out_tvalid_0,
  output wire [255:0] adc_data_out_tdata_1,  // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  output wire         adc_data_out_tvalid_1,
  output wire [255:0] adc_data_out_tdata_2,  // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  output wire         adc_data_out_tvalid_2,
  output wire [255:0] adc_data_out_tdata_3,  // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  output wire         adc_data_out_tvalid_3,

  // DAC
  input  wire [255:0] dac_data_in_tdata_0,   // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  output wire         dac_data_in_tready_0,
  input  wire         dac_data_in_tvalid_0,
  input  wire [255:0] dac_data_in_tdata_1,   // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  output wire         dac_data_in_tready_1,
  input  wire         dac_data_in_tvalid_1,
  input  wire [255:0] dac_data_in_tdata_2,   // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  output wire         dac_data_in_tready_2,
  input  wire         dac_data_in_tvalid_2,
  input  wire [255:0] dac_data_in_tdata_3,   // Packed [Q7,I7, ... , Q0,I0] with Q in MSBs
  output wire         dac_data_in_tready_3,
  input  wire         dac_data_in_tvalid_3,

  //---------------------------------------------------------------------------
  // Miscellaneous
  //---------------------------------------------------------------------------

  // Invert I/Q control signals from RFDC to DSP chain.
  input  wire [3:0] invert_adc_iq_rclk2,
  input  wire [3:0] invert_dac_iq_rclk2,

  // Control/status vectors from/to RFDC.
  // Notice these are all in the s_axi_config_clk domain.
  output wire [9:0] dsp_info_sclk,
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

  // ADC data interface from RFDC.
  wire [127:0] adc_data_in_i_tdata       [0:NUM_ADC_CHANNELS-1]; // 8 SPC (I)
  wire [127:0] adc_data_in_q_tdata       [0:NUM_ADC_CHANNELS-1]; // 8 SPC (Q)
  wire [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tready;
  wire [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tready;
  wire [NUM_ADC_CHANNELS-1:0] adc_data_in_i_tvalid;
  wire [NUM_ADC_CHANNELS-1:0] adc_data_in_q_tvalid;
  // DAC data interface to RFDC.
  wire [255:0] dac_data_out_tdata        [0:NUM_DAC_CHANNELS-1]; // 8 SPC (I + Q)
  wire [NUM_DAC_CHANNELS-1:0] dac_data_out_tready;
  wire [NUM_DAC_CHANNELS-1:0] dac_data_out_tvalid;

  // ADC data interface to user.
  wire [255:0] adc_data_out_tdata        [0:NUM_ADC_CHANNELS-1]; // 8 SPC (I + Q)
  wire [NUM_ADC_CHANNELS-1:0] adc_data_out_tready;
  wire [NUM_ADC_CHANNELS-1:0] adc_data_out_tvalid;
  // DAC data interface from user.
  wire [255:0] dac_data_in_tdata_preswap [0:NUM_DAC_CHANNELS-1]; // 8 SPC (I + Q)
  wire [255:0] dac_data_in_tdata         [0:NUM_DAC_CHANNELS-1]; // 8 SPC (I + Q)
  wire [NUM_DAC_CHANNELS-1:0] dac_data_in_tready;
  wire [NUM_DAC_CHANNELS-1:0] dac_data_in_tvalid;

  wire [15:0] axi_status;


  //---------------------------------------------------------------------------
  // Resets, Debug and Misc.
  //---------------------------------------------------------------------------

  // Group all these status bits together. They don't toggle frequently so data
  // coherency is not an issue here.
  // Using constants for DB0 since the bits are the 16 LSBs in a 32-bit vector.
  // DB1 simply uses the 16 MSBs when wiring the status vector.
  assign axi_status[USER_ADC_TREADY_MSB  :USER_ADC_TREADY  ] = adc_data_out_tready[1:0];
  assign axi_status[USER_ADC_TVALID_MSB  :USER_ADC_TVALID  ] = adc_data_out_tvalid[1:0];
  assign axi_status[RFDC_ADC_I_TVALID_MSB:RFDC_ADC_I_TVALID] = adc_data_in_i_tvalid[1:0];
  assign axi_status[RFDC_ADC_Q_TVALID_MSB:RFDC_ADC_Q_TVALID] = adc_data_in_q_tvalid[1:0];
  assign axi_status[RFDC_ADC_I_TREADY_MSB:RFDC_ADC_I_TREADY] = adc_data_in_i_tready[1:0];
  assign axi_status[RFDC_ADC_Q_TREADY_MSB:RFDC_ADC_Q_TREADY] = adc_data_in_q_tready[1:0];
  assign axi_status[RFDC_DAC_TVALID_MSB  :RFDC_DAC_TVALID  ] = dac_data_out_tvalid[1:0];
  assign axi_status[RFDC_DAC_TREADY_MSB  :RFDC_DAC_TREADY  ] = dac_data_out_tready[1:0];

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

  // This RF core always consumes 8 SPC from the gearbox per I/Q signal
  assign rfdc_info_sclk[RFDC_INFO_SPC_RX_MSB:RFDC_INFO_SPC_RX] = $clog2(8);
  assign rfdc_info_sclk[RFDC_INFO_SPC_TX_MSB:RFDC_INFO_SPC_TX] = $clog2(16);
  // This RF core module contains no additional resampling
  assign rfdc_info_sclk[RFDC_INFO_XTRA_RESAMP_MSB:RFDC_INFO_XTRA_RESAMP] = 4'd1;

  //---------------------------------------------------------------------------
  // ADC Post-Processing
  //---------------------------------------------------------------------------

  // Data comes from the RFDC as 8 SPC, separate streams for each channel and
  // I/Q.
  assign adc_data_in_i_tdata[0]  = adc_data_in_i_tdata_0;
  assign adc_data_in_q_tdata[0]  = adc_data_in_q_tdata_0;
  assign adc_data_in_i_tdata[1]  = adc_data_in_i_tdata_1;
  assign adc_data_in_q_tdata[1]  = adc_data_in_q_tdata_1;
  assign adc_data_in_i_tdata[2]  = adc_data_in_i_tdata_2;
  assign adc_data_in_q_tdata[2]  = adc_data_in_q_tdata_2;
  assign adc_data_in_i_tdata[3]  = adc_data_in_i_tdata_3;
  assign adc_data_in_q_tdata[3]  = adc_data_in_q_tdata_3;

  assign adc_data_in_i_tready_0  = adc_data_in_i_tready[0];
  assign adc_data_in_i_tvalid[0] = adc_data_in_i_tvalid_0;
  assign adc_data_in_q_tready_0  = adc_data_in_q_tready[0];
  assign adc_data_in_q_tvalid[0] = adc_data_in_q_tvalid_0;
  assign adc_data_in_i_tready_1  = adc_data_in_i_tready[1];
  assign adc_data_in_i_tvalid[1] = adc_data_in_i_tvalid_1;
  assign adc_data_in_q_tready_1  = adc_data_in_q_tready[1];
  assign adc_data_in_q_tvalid[1] = adc_data_in_q_tvalid_1;
  assign adc_data_in_i_tready_2  = adc_data_in_i_tready[2];
  assign adc_data_in_i_tvalid[2] = adc_data_in_i_tvalid_2;
  assign adc_data_in_q_tready_2  = adc_data_in_q_tready[2];
  assign adc_data_in_q_tvalid[2] = adc_data_in_q_tvalid_2;
  assign adc_data_in_i_tready_3  = adc_data_in_i_tready[3];
  assign adc_data_in_i_tvalid[3] = adc_data_in_i_tvalid_3;
  assign adc_data_in_q_tready_3  = adc_data_in_q_tready[3];
  assign adc_data_in_q_tvalid[3] = adc_data_in_q_tvalid_3;

  // ADC Data from the RFDC arrives here as 8 SPC with separate I and Q
  // streams. It leaves the adc_full_rate_bd as 8 SPC with I and Q packed into
  // a single 256 bit word.
  genvar adc_num;
  generate
  for (adc_num=0; adc_num < (NUM_ADC_CHANNELS); adc_num = adc_num + 1)
    begin : adc_gen
      adc_full_bd adc_full_bd_gen (
        .enable_data_to_repacker_rclk  (adc_enable_data_rclk),
        .rfdc_adc_axi_resetn_rclk      (adc_rfdc_axi_resetn_rclk),
        .rfdc_clk                      (rfdc_clk),
        .swap_iq_rclk                  (invert_adc_iq_rclk2 [adc_num]),
        .adc_q_data_in_tvalid          (adc_data_in_q_tvalid[adc_num]),
        .adc_q_data_in_tready          (adc_data_in_q_tready[adc_num]),
        .adc_q_data_in_tdata           (adc_data_in_q_tdata [adc_num]),
        .adc_i_data_in_tvalid          (adc_data_in_i_tvalid[adc_num]),
        .adc_i_data_in_tready          (adc_data_in_i_tready[adc_num]),
        .adc_i_data_in_tdata           (adc_data_in_i_tdata [adc_num]),
        .adc_data_out_tvalid           (adc_data_out_tvalid [adc_num]),
        .adc_data_out_tdata            (adc_data_out_tdata  [adc_num])
      );
    end
  endgenerate

  // Data is released to the user as 8 SPC, separate streams for each channel.
  assign adc_data_out_tdata_0 = adc_data_out_tdata[0];
  assign adc_data_out_tdata_1 = adc_data_out_tdata[1];
  assign adc_data_out_tdata_2 = adc_data_out_tdata[2];
  assign adc_data_out_tdata_3 = adc_data_out_tdata[3];

  // There is no tready going to the ADC (one has to be always ready for ADC
  // data), but it is still a component of the axi_status vector as a generic
  // AXI stream status. Report 1'b1 to the status vector consistent with being
  // always ready
  assign adc_data_out_tready[0] = 1'b1;
  assign adc_data_out_tvalid_0  = adc_data_out_tvalid[0];
  assign adc_data_out_tready[1] = 1'b1;
  assign adc_data_out_tvalid_1  = adc_data_out_tvalid[1];
  assign adc_data_out_tready[2] = 1'b1;
  assign adc_data_out_tvalid_2  = adc_data_out_tvalid[2];
  assign adc_data_out_tready[3] = 1'b1;
  assign adc_data_out_tvalid_3  = adc_data_out_tvalid[3];

  //---------------------------------------------------------------------------
  // DAC Pre-Processing
  //---------------------------------------------------------------------------

  // Data comes from the user as 8 SPC, separate streams for each channel.
  assign dac_data_in_tdata_preswap[0] = dac_data_in_tdata_0;
  assign dac_data_in_tdata_preswap[1] = dac_data_in_tdata_1;
  assign dac_data_in_tdata_preswap[2] = dac_data_in_tdata_2;
  assign dac_data_in_tdata_preswap[3] = dac_data_in_tdata_3;

  assign dac_data_in_tready_0  = dac_data_in_tready[0];
  assign dac_data_in_tvalid[0] = dac_data_in_tvalid_0;
  assign dac_data_in_tready_1  = dac_data_in_tready[1];
  assign dac_data_in_tvalid[1] = dac_data_in_tvalid_1;
  assign dac_data_in_tready_2  = dac_data_in_tready[2];
  assign dac_data_in_tvalid[2] = dac_data_in_tvalid_2;
  assign dac_data_in_tready_3  = dac_data_in_tready[3];
  assign dac_data_in_tvalid[3] = dac_data_in_tvalid_3;

  genvar dac_num;
  generate
  for (dac_num=0; dac_num < (NUM_DAC_CHANNELS); dac_num = dac_num + 1)
    begin : dac_swap_gen
      //IO and Q0 swap
      assign dac_data_in_tdata[dac_num][15:00] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][31:16]) : (dac_data_in_tdata_preswap[dac_num][15:0]);
      assign dac_data_in_tdata[dac_num][31:16] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][15:00]) : (dac_data_in_tdata_preswap[dac_num][31:16]);

      //I1 and Q1 swap
      assign dac_data_in_tdata[dac_num][47:32] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][63:48]) : (dac_data_in_tdata_preswap[dac_num][47:32]);
      assign dac_data_in_tdata[dac_num][63:48] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][47:32]) : (dac_data_in_tdata_preswap[dac_num][63:48]);

      //I2 and Q2 swap
      assign dac_data_in_tdata[dac_num][79:64] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][95:80]) : (dac_data_in_tdata_preswap[dac_num][79:64]);
      assign dac_data_in_tdata[dac_num][95:80] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][79:64]) : (dac_data_in_tdata_preswap[dac_num][95:80]);

      //I3 and Q3 swap
      assign dac_data_in_tdata[dac_num][111:96] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][127:112]) : (dac_data_in_tdata_preswap[dac_num][111:96]);
      assign dac_data_in_tdata[dac_num][127:112] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][111:96]) : (dac_data_in_tdata_preswap[dac_num][127:112]);

      //I4 and Q4 swap
      assign dac_data_in_tdata[dac_num][143:128] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][159:144]) : (dac_data_in_tdata_preswap[dac_num][143:128]);
      assign dac_data_in_tdata[dac_num][159:144] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][143:128]) : (dac_data_in_tdata_preswap[dac_num][159:144]);

      //I5 and Q5 swap
      assign dac_data_in_tdata[dac_num][175:160] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][191:176]) : (dac_data_in_tdata_preswap[dac_num][175:160]);
      assign dac_data_in_tdata[dac_num][191:176] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][175:160]) : (dac_data_in_tdata_preswap[dac_num][191:176]);

      //I6 and Q6 swap
      assign dac_data_in_tdata[dac_num][207:192] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][223:208]) : (dac_data_in_tdata_preswap[dac_num][207:192]);
      assign dac_data_in_tdata[dac_num][223:208] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][207:192]) : (dac_data_in_tdata_preswap[dac_num][223:208]);

      //I7 and Q7 swap
      assign dac_data_in_tdata[dac_num][239:224] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][255:240]) : (dac_data_in_tdata_preswap[dac_num][239:224]);
      assign dac_data_in_tdata[dac_num][255:240] = invert_dac_iq_rclk2[dac_num] ?
        (dac_data_in_tdata_preswap[dac_num][239:224]) : (dac_data_in_tdata_preswap[dac_num][255:240]);

    end
  endgenerate

  // These streams are then connected to data out, no need for a bd, and form a single
  // stream per channel, 8 SPC, packed: MSB [Sample7Q, Sample7I, ... ,
  // Sample0Q, Sample0I] LSB.
  generate
  for (dac_num=0; dac_num < (NUM_DAC_CHANNELS); dac_num = dac_num + 1)
    begin : dac_gen
      assign dac_data_out_tdata[dac_num] = dac_data_in_tdata[dac_num];
      assign dac_data_out_tvalid[dac_num] = dac_data_in_tvalid[dac_num];
      assign dac_data_in_tready[dac_num] = dac_data_out_tready[dac_num];
    end
  endgenerate

  // Data is released to the RFDC as 8 SPC, separate streams per channel (I/Q
  // together).
  assign dac_data_out_tdata_0 = dac_data_out_tdata[0];
  assign dac_data_out_tdata_1 = dac_data_out_tdata[1];
  assign dac_data_out_tdata_2 = dac_data_out_tdata[2];
  assign dac_data_out_tdata_3 = dac_data_out_tdata[3];

  assign dac_data_out_tready[0] = dac_data_out_tready_0;
  assign dac_data_out_tvalid_0  = dac_data_out_tvalid[0];
  assign dac_data_out_tready[1] = dac_data_out_tready_1;
  assign dac_data_out_tvalid_1  = dac_data_out_tvalid[1];
  assign dac_data_out_tready[2] = dac_data_out_tready_2;
  assign dac_data_out_tvalid_2  = dac_data_out_tvalid[2];
  assign dac_data_out_tready[3] = dac_data_out_tready_3;
  assign dac_data_out_tvalid_3  = dac_data_out_tvalid[3];


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
//      <value name="RF_CORE_FULL_CURRENT_VERSION_MINOR"           integer="0"/>
//      <value name="RF_CORE_FULL_CURRENT_VERSION_BUILD"           integer="0"/>
//      <value name="RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_MAJOR" integer="1"/>
//      <value name="RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_MINOR" integer="0"/>
//      <value name="RF_CORE_FULL_OLDEST_COMPATIBLE_VERSION_BUILD" integer="0"/>
//      <value name="RF_CORE_FULL_VERSION_LAST_MODIFIED_TIME"      integer="0x22062900"/>
//    </enumeratedtype>
//  </group>
//</regmap>
//XmlParse xml_off

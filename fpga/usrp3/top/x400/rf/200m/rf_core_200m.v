//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rf_core_200m
//
// Description:
//
//   Implementation of rf_core with 200 MHz bandwidth. It presents an interface
//   that inputs/outputs 2 samples per cycle. This version is implemented by
//   instantiating rf_core_400m and adding up-conversion and down-conversion
//   filters.
//

`default_nettype none

module rf_core_200m (

  //---------------------------------------------------------------------------
  // Clocking
  //---------------------------------------------------------------------------

  // Main Clock Inputs
  input  wire rfdc_clk,
  input  wire rfdc_clk_2x,
  input  wire data_clk,
  input  wire data_clk_2x,

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

  // DAC
  output wire [255:0] dac_data_out_tdata_0,
  input  wire         dac_data_out_tready_0,
  output wire         dac_data_out_tvalid_0,
  output wire [255:0] dac_data_out_tdata_1,
  input  wire         dac_data_out_tready_1,
  output wire         dac_data_out_tvalid_1,

  //---------------------------------------------------------------------------
  // User Data Interface
  //---------------------------------------------------------------------------
  // All ports here are in the data_clk domain.

  // ADC
  output wire [63:0] adc_data_out_tdata_0,  // Packed {Q1,I1,Q0,I0}
  output wire        adc_data_out_tvalid_0,
  output wire [63:0] adc_data_out_tdata_1,  // Packed {Q1,I1,Q0,I0}
  output wire        adc_data_out_tvalid_1,

  // DAC
  input  wire [63:0] dac_data_in_tdata_0,   // Packed {Q1,I1,Q0,I0} with Q in MSBs
  output wire        dac_data_in_tready_0,
  input  wire        dac_data_in_tvalid_0,
  input  wire [63:0] dac_data_in_tdata_1,   // Packed {Q1,I1,Q0,I0} with Q in MSBs
  output wire        dac_data_in_tready_1,
  input  wire        dac_data_in_tvalid_1,


  //---------------------------------------------------------------------------
  // Miscellaneous
  //---------------------------------------------------------------------------

  // Invert I/Q control signals from RFDC to DSP chain.
  input  wire [3:0] invert_adc_iq_rclk2,
  input  wire [3:0] invert_dac_iq_rclk2,

  // Control/status vectors from/to RFDC.
  // Notice these are all in the s_axi_config_clk domain.
  output wire  [9:0] dsp_info_sclk,
  output wire [15:0] axi_status_sclk,
  output wire [15:0] rfdc_info_sclk,

  // Resets.
  input wire adc_data_out_resetn_dclk,
  input wire adc_enable_data_rclk,
  input wire adc_rfdc_axi_resetn_rclk,
  input wire dac_data_in_resetn_dclk,
  input wire dac_data_in_resetn_dclk2x,
  input wire dac_data_in_resetn_rclk,
  input wire fir_resetn_rclk2x,

  // Version (Constant)
  output wire [95:0] version_info
);

  `include "../../regmap/x410/rfdc_regs_regmap_utils.vh"

  //---------------------------------------------------------------------------
  // 400 MHz RF Core
  //---------------------------------------------------------------------------

  wire [127:0] adc_400m_tdata_0;
  wire         adc_400m_tvalid_0;
  wire [127:0] adc_400m_tdata_1;
  wire         adc_400m_tvalid_1;
  wire [127:0] dac_400m_tdata_0;
  wire         dac_400m_tvalid_0;
  wire [127:0] dac_400m_tdata_1;
  wire         dac_400m_tvalid_1;

  wire [ 15:0] dsp_info_sclk_400m;

  rf_core_400m rf_core_400m_i (
    .rfdc_clk                  (rfdc_clk),
    .rfdc_clk_2x               (rfdc_clk_2x),
    .data_clk                  (data_clk),
    .data_clk_2x               (data_clk_2x),
    .s_axi_config_clk          (s_axi_config_clk),
    .adc_data_in_i_tdata_0     (adc_data_in_i_tdata_0),
    .adc_data_in_i_tready_0    (adc_data_in_i_tready_0),
    .adc_data_in_i_tvalid_0    (adc_data_in_i_tvalid_0),
    .adc_data_in_q_tdata_0     (adc_data_in_q_tdata_0),
    .adc_data_in_q_tready_0    (adc_data_in_q_tready_0),
    .adc_data_in_q_tvalid_0    (adc_data_in_q_tvalid_0),
    .adc_data_in_i_tdata_1     (adc_data_in_i_tdata_1),
    .adc_data_in_i_tready_1    (adc_data_in_i_tready_1),
    .adc_data_in_i_tvalid_1    (adc_data_in_i_tvalid_1),
    .adc_data_in_q_tdata_1     (adc_data_in_q_tdata_1),
    .adc_data_in_q_tready_1    (adc_data_in_q_tready_1),
    .adc_data_in_q_tvalid_1    (adc_data_in_q_tvalid_1),
    .dac_data_out_tdata_0      (dac_data_out_tdata_0),
    .dac_data_out_tready_0     (dac_data_out_tready_0),
    .dac_data_out_tvalid_0     (dac_data_out_tvalid_0),
    .dac_data_out_tdata_1      (dac_data_out_tdata_1),
    .dac_data_out_tready_1     (dac_data_out_tready_1),
    .dac_data_out_tvalid_1     (dac_data_out_tvalid_1),
    .adc_data_out_tdata_0      (adc_400m_tdata_0),
    .adc_data_out_tvalid_0     (adc_400m_tvalid_0),
    .adc_data_out_tdata_1      (adc_400m_tdata_1),
    .adc_data_out_tvalid_1     (adc_400m_tvalid_1),
    .dac_data_in_tdata_0       (dac_400m_tdata_0),
    .dac_data_in_tready_0      (),
    .dac_data_in_tvalid_0      (dac_400m_tvalid_0),
    .dac_data_in_tdata_1       (dac_400m_tdata_1),
    .dac_data_in_tready_1      (),
    .dac_data_in_tvalid_1      (dac_400m_tvalid_1),
    .invert_adc_iq_rclk2       (invert_adc_iq_rclk2),
    .invert_dac_iq_rclk2       (invert_dac_iq_rclk2),
    .dsp_info_sclk             (dsp_info_sclk_400m),
    .axi_status_sclk           (axi_status_sclk),
    .adc_data_out_resetn_dclk  (adc_data_out_resetn_dclk),
    .adc_enable_data_rclk      (adc_enable_data_rclk),
    .adc_rfdc_axi_resetn_rclk  (adc_rfdc_axi_resetn_rclk),
    .dac_data_in_resetn_dclk   (dac_data_in_resetn_dclk),
    .dac_data_in_resetn_dclk2x (dac_data_in_resetn_dclk2x),
    .dac_data_in_resetn_rclk   (dac_data_in_resetn_rclk),
    .fir_resetn_rclk2x         (fir_resetn_rclk2x),
    .version_info              (version_info)
  );

  assign dsp_info_sclk = dsp_info_sclk_400m;

  // This RF core always consumes 8 SPC from the gearbox per I/Q signal
  assign rfdc_info_sclk[RFDC_INFO_SPC_RX_MSB:RFDC_INFO_SPC_RX] = $clog2(8);
  assign rfdc_info_sclk[RFDC_INFO_SPC_TX_MSB:RFDC_INFO_SPC_TX] = $clog2(16);
  assign rfdc_info_sclk[RFDC_INFO_XTRA_RESAMP_MSB:RFDC_INFO_XTRA_RESAMP] = 4'd6;


  //---------------------------------------------------------------------------
  // ADC Down-conversion
  //---------------------------------------------------------------------------

  rf_down_4to2 #(
    .NUM_CHANNELS (2)
  ) rf_down_4to2_i (
    .clk      (data_clk),
    .clk_2x   (data_clk_2x),
    .rst      (~adc_data_out_resetn_dclk),
    .rst_2x   (~adc_data_out_resetn_dclk), // 1x clk reset is safe to use
    .i_tdata  ({ adc_400m_tdata_1,  adc_400m_tdata_0 }),
    .i_tvalid ({ adc_400m_tvalid_1, adc_400m_tvalid_0 }),
    .o_tdata  ({ adc_data_out_tdata_1,  adc_data_out_tdata_0 }),
    .o_tvalid ({ adc_data_out_tvalid_1, adc_data_out_tvalid_0 })
  );


  //---------------------------------------------------------------------------
  // DAC Up-conversion
  //---------------------------------------------------------------------------

  assign dac_data_in_tready_0 = 1'b1;
  assign dac_data_in_tready_1 = 1'b1;

  rf_up_2to4 #(
    .NUM_CHANNELS (2)
  ) rf_up_2to4_i (
    .clk      (data_clk),
    .clk_2x   (data_clk_2x),
    .rst      (~dac_data_in_resetn_dclk),
    .rst_2x   (~dac_data_in_resetn_dclk2x),
    .i_tdata  ({ dac_data_in_tdata_1, dac_data_in_tdata_0 }),
    .i_tvalid ({ dac_data_in_tvalid_1, dac_data_in_tvalid_0 }),
    .o_tdata  ({ dac_400m_tdata_1,  dac_400m_tdata_0 }),
    .o_tvalid ({ dac_400m_tvalid_1, dac_400m_tvalid_0 })
  );

endmodule

`default_nettype wire

//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fe_control
//
// Description: Handle the front end control from the radio settings bus.
// The module gets generated NUM_CHANNELS times to give independent control to
// the individual channels.
//

module fe_control #(
  parameter NUM_CHANNELS = 2,
  parameter [7:0] SR_FE_CHAN_OFFSET = 16,
  parameter [7:0] SR_TX_FE_BASE = 192,
  parameter [7:0] SR_RX_FE_BASE = 200
)(
  input clk, input reset,
  // Commands from Radio Core
  input  set_stb, input [7:0] set_addr, input  [31:0] set_data,
  input  time_sync,
  // Radio datapath
  input  [NUM_CHANNELS-1:0] tx_stb, input [32*NUM_CHANNELS-1:0] tx_data_in, output [32*NUM_CHANNELS-1:0] tx_data_out,
  output [NUM_CHANNELS-1:0] rx_stb, input [32*NUM_CHANNELS-1:0] rx_data_in, output [32*NUM_CHANNELS-1:0] rx_data_out
);

  genvar i;
  generate for (i = 0; i < NUM_CHANNELS; i = i + 1)
    begin
      localparam SR_TX_OFFSET_I         = SR_TX_FE_BASE + SR_FE_CHAN_OFFSET*i + 0;
      localparam SR_TX_OFFSET_Q         = SR_TX_FE_BASE + SR_FE_CHAN_OFFSET*i + 1;
      localparam SR_TX_MAG_CORRECTION   = SR_TX_FE_BASE + SR_FE_CHAN_OFFSET*i + 2;
      localparam SR_TX_PHASE_CORRECTION = SR_TX_FE_BASE + SR_FE_CHAN_OFFSET*i + 3;
      localparam SR_TX_MUX              = SR_TX_FE_BASE + SR_FE_CHAN_OFFSET*i + 4;

      tx_frontend_gen3 #(
        .SR_OFFSET_I(SR_TX_OFFSET_I), .SR_OFFSET_Q(SR_TX_OFFSET_Q),.SR_MAG_CORRECTION(SR_TX_MAG_CORRECTION),
        .SR_PHASE_CORRECTION(SR_TX_PHASE_CORRECTION), .SR_MUX(SR_TX_MUX),
        .BYPASS_DC_OFFSET_CORR(0), .BYPASS_IQ_COMP(0),
        .DEVICE("7SERIES")
      ) tx_fe_corr_i (
        .clk(clk), .reset(reset),
        .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
        .tx_stb(tx_stb[i]), .tx_i(tx_data_in[32+(32*i)-1:16+(32*i)]), .tx_q(tx_data_in[16+(32*i)-1:(32*i)]),
        .dac_stb(), .dac_i(tx_data_out[32+(32*i)-1:16+(32*i)]), .dac_q(tx_data_out[16+(32*i)-1:(32*i)])
      );

      localparam SR_RX_MAG_CORRECTION   = SR_RX_FE_BASE + SR_FE_CHAN_OFFSET*i + 0;
      localparam SR_RX_PHASE_CORRECTION = SR_RX_FE_BASE + SR_FE_CHAN_OFFSET*i + 1;
      localparam SR_RX_OFFSET_I         = SR_RX_FE_BASE + SR_FE_CHAN_OFFSET*i + 2;
      localparam SR_RX_OFFSET_Q         = SR_RX_FE_BASE + SR_FE_CHAN_OFFSET*i + 3;
      localparam SR_RX_IQ_MAPPING       = SR_RX_FE_BASE + SR_FE_CHAN_OFFSET*i + 4;
      localparam SR_RX_HET_PHASE_INCR   = SR_RX_FE_BASE + SR_FE_CHAN_OFFSET*i + 5;

      rx_frontend_gen3 #(
        .SR_MAG_CORRECTION(SR_RX_MAG_CORRECTION), .SR_PHASE_CORRECTION(SR_RX_PHASE_CORRECTION), .SR_OFFSET_I(SR_RX_OFFSET_I),
        .SR_OFFSET_Q(SR_RX_OFFSET_Q), .SR_IQ_MAPPING(SR_RX_IQ_MAPPING), .SR_HET_PHASE_INCR(SR_RX_HET_PHASE_INCR),
        .BYPASS_DC_OFFSET_CORR(0), .BYPASS_IQ_COMP(0), .BYPASS_REALMODE_DSP(0),
        .DEVICE("7SERIES")
      ) rx_fe_corr_i (
        .clk(clk), .reset(reset), .sync_in(time_sync),
        .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
        .adc_stb(1'b1), .adc_i(rx_data_in[32+(32*i)-1:16+(32*i)]), .adc_q(rx_data_in[16+(32*i)-1:(32*i)]),
        .rx_stb(rx_stb[i]), .rx_i(rx_data_out[32+(32*i)-1:16+(32*i)]), .rx_q(rx_data_out[16+(32*i)-1:(32*i)])
      );
    end
  endgenerate

endmodule

//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rx_dec3
//
// Description:
//
//   RX decimate-by-3 module. Wraps the FIR Filter IP (2 SPC input),
//   rounds/clips the full-precision 34-bit FIR output back to 16-bit per path,
//   and outputs 2 IQ samples per valid in the data_clk domain.
//

module rx_dec3 (
  input  logic        rfdc_clk,
  input  logic        data_clk,
  input  logic        pll_ref_clk,
  input  logic        reset_pulse_dclk,

  // Input DATA channel (AXI4-Stream slave, 2 SPC @ rfdc_clk)
  input  logic [63:0] adc_data_in_tdata,
  input  logic        adc_data_in_tvalid,
  output logic        adc_data_in_tready,

  // Output DATA channel (2 SPC @ data_clk)
  output logic [63:0] adc_data_out_tdata,
  output logic        adc_data_out_tvalid
);

  //---------------------------------------------------------------------------
  // Constants
  //---------------------------------------------------------------------------
  localparam int NUM_PATHS      = 2;   // Output data paths from FIR (I and Q)
  localparam int FIR_OUT_WIDTH  = 34;  // Full-precision output per path
  localparam int FIR_PATH_WIDTH = 40;  // Padded path width in tdata (8-bit boundary)
  localparam int DATA_WIDTH     = 16;  // Desired output sample width
  // Coefficient DC gain ~= 2^17, so 17 fractional bits are rounded away
  // and 1 bit of headroom is clipped (saturation).
  localparam int CLIP_BITS      = 1;

  //---------------------------------------------------------------------------
  // Reset generation
  //---------------------------------------------------------------------------
  logic resetn_dclk;
  logic resetn_rclk;

  rf_reset rf_reset_i (
    .DataClk     (data_clk),
    .PllRefClk   (pll_ref_clk),
    .RfClk       (rfdc_clk),
    .RfClk2x     (1'b0),
    .DataClk2x   (1'b0),
    .dTimedReset (1'b0),
    .dSwReset    (reset_pulse_dclk),
    .dReset_n    (resetn_dclk),
    .d2Reset_n   (),
    .r2Reset_n   (),
    .rAxiReset_n (),
    .rReset_n    (resetn_rclk)
  );

  //---------------------------------------------------------------------------
  // FIR Filter
  //---------------------------------------------------------------------------
  // FIR filter output signals
  logic [NUM_PATHS-1:0][FIR_PATH_WIDTH-1:0] fir_tdata;
  logic                                     fir_tvalid;

  fir_dec3_2spc_wrapper fir_dec3_2spc_wrapper_i (
    .aclk               (rfdc_clk),
    .aresetn            (resetn_rclk),
    .s_axis_data_tdata  (adc_data_in_tdata),
    .s_axis_data_tvalid (adc_data_in_tvalid),
    .s_axis_data_tready (adc_data_in_tready),
    .m_axis_data_tdata  (fir_tdata),
    .m_axis_data_tvalid (fir_tvalid),
    .m_axis_data_tuser  ()
  );


  //---------------------------------------------------------------------------
  // Clipping and Rounding
  //---------------------------------------------------------------------------
  // Round and clip each output path from 34-bit full precision to 16-bit.
  // Strips the 6-bit AXI padding per slot and removes the 18 bits of
  // coefficient growth (17 fractional bits rounded + 1 bit clipped).
  logic [NUM_PATHS-1:0][DATA_WIDTH-1:0] rc_tdata;
  logic [NUM_PATHS-1:0] rc_tvalid;

  generate
    for (genvar i = 0; i < NUM_PATHS; i++) begin : gen_round_clip
      axi_round_and_clip #(
        .WIDTH_IN  (FIR_OUT_WIDTH),
        .WIDTH_OUT (DATA_WIDTH),
        .CLIP_BITS (CLIP_BITS)
      ) axi_round_and_clip_i (
        .clk      (rfdc_clk),
        .reset    (~resetn_rclk),
        .i_tdata  (fir_tdata[i][FIR_OUT_WIDTH-1:0]),
        .i_tlast  (1'b0),
        .i_tvalid (fir_tvalid),
        .i_tready (),
        .o_tdata  (rc_tdata[i]),
        .o_tlast  (),
        .o_tvalid (rc_tvalid[i]),
        .o_tready (1'b1)
      );
    end
  endgenerate

  //---------------------------------------------------------------------------
  // CDC
  //---------------------------------------------------------------------------
  // reuse wider module to do CDC and fill unused samples with zero
  localparam int CDC_WIDTH = 24;
  logic [NUM_PATHS*2-1:0][CDC_WIDTH-1:0] cdc_tdata_in;  // 2 IQ samples
  logic [NUM_PATHS*4-1:0][CDC_WIDTH-1:0] cdc_tdata_out; // 4 IQ samples

  // extend input data to match CDC module width
  always_comb begin
    cdc_tdata_in[0] = rc_tdata[0];
    cdc_tdata_in[1] = rc_tdata[1];
    cdc_tdata_in[2] = '0;
    cdc_tdata_in[3] = '0;
  end

  adc_gearbox_2x4 adc_gearbox_2x4_i (
    .Clk1x          (data_clk),
    .Clk3x          (rfdc_clk),
    .ac1Reset_n     (resetn_dclk),
    .ac3Reset_n     (resetn_rclk),
    .c3DataIn       (cdc_tdata_in),
    .c3DataValidIn  (rc_tvalid[0]),
    .c1DataOut      (cdc_tdata_out),
    .c1DataValidOut (adc_data_out_tvalid)
  );

  // extract the samples of interest from the CDC module output
  always_comb begin
    adc_data_out_tdata[15: 0]  = cdc_tdata_out[0][DATA_WIDTH-1:0];
    adc_data_out_tdata[31:16]  = cdc_tdata_out[1][DATA_WIDTH-1:0];
    adc_data_out_tdata[47:32]  = cdc_tdata_out[4][DATA_WIDTH-1:0];
    adc_data_out_tdata[63:48]  = cdc_tdata_out[5][DATA_WIDTH-1:0];
  end

endmodule : rx_dec3

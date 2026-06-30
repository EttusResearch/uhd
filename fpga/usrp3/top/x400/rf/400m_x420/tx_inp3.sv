//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: tx_inp3
//
// Description:
//
//   TX interpolate-by-3 module. Gearboxes 4 SPC @ data_clk down to 2 SPC @
//   data_clk_2x, feeds the FIR interpolation filter (×3), rounds/clips the
//   35-bit output back to 16-bit, and gearboxes from 6 SPC @ data_clk_2x to
//   4 SPC @ rfdc_clk.
//
//   This module does not support backpressure on the data path. It is designed
//   for full throughput.
//
//   Clock relationships (all derived from pll_ref_clk via the same MMCM):
//     data_clk  = N × pll_ref_clk   (slowest clock, used as Clk1x by the gearbox)
//     data_clk_2x  = 2 × data_clk   (user-side data clock; FIR runs here)
//     rfdc_clk  = 3 × data_clk      (RFDC sample clock; gearbox output domain)
//

module tx_inp3 (
  input  logic         pll_ref_clk,
  input  logic         data_clk,
  input  logic         data_clk_2x,
  input  logic         rfdc_clk,

  input  logic         reset_pulse_dclk,

  // Input DATA channel (4 SPC @ data_clk)
  input  logic [127:0] dac_data_in_tdata,
  input  logic         dac_data_in_tvalid,

  // Output DATA channel (4 SPC @ rfdc_clk)
  output logic [127:0] dac_data_out_tdata,
  output logic         dac_data_out_tvalid
);

  //---------------------------------------------------------------------------
  // Reset generation
  //---------------------------------------------------------------------------
  // rf_reset aligns all domain resets to PllRefClk edges so that the latency
  // through the module is deterministic regardless of clock phase at startup.
  logic resetn_dclk;
  logic resetn_d2clk;

  rf_reset rf_reset_i (
    .DataClk     (data_clk),
    .PllRefClk   (pll_ref_clk),
    .RfClk       (1'b0),
    .RfClk2x     (1'b0),
    .DataClk2x   (data_clk_2x),
    .dTimedReset (1'b0),
    .dSwReset    (reset_pulse_dclk),
    .dReset_n    (resetn_dclk),
    .d2Reset_n   (resetn_d2clk),
    .r2Reset_n   (),
    .rAxiReset_n (),
    .rReset_n    ()
  );

  localparam int NUM_OUT_PATHS  = 12;  // Output data paths from FIR (2 I/Q × 6 SPC)
  localparam int FIR_OUT_WIDTH  = 35;  // Full-precision output per path
  localparam int FIR_PATH_WIDTH = 40;  // Padded path width in tdata (8-bit boundary)
  localparam int DATA_WIDTH     = 16;  // Desired output sample width
  // Each polyphase branch's coefficient DC gain ~= 2^17, so 17 fractional
  // bits are rounded away and 2 bits of headroom are clipped (saturation).
  localparam int CLIP_BITS      = 2;

  //---------------------------------------------------------------------------
  // Data serialization to faster clock domain
  //---------------------------------------------------------------------------
  // gearbox 4 SPC @ data_clk to 2 SPC @ data_clk_2x
  logic [63:0] serial_tdata_out;
  logic        serial_tvalid_out;

  dac_gearbox_4x2 dac_gearbox_4x2_i (
    .clk1x          (data_clk),
    .reset_n_1x     (resetn_dclk),
    .data_in_1x     (dac_data_in_tdata),
    .valid_in_1x    (dac_data_in_tvalid),
    .ready_out_1x   (),
    .clk2x          (data_clk_2x),
    .data_out_2x    (serial_tdata_out),
    .valid_out_2x   (serial_tvalid_out)
  );

  //---------------------------------------------------------------------------
  // FIR interpolation ×3 @ data_clk_2x
  //---------------------------------------------------------------------------
  logic [NUM_OUT_PATHS-1:0][FIR_PATH_WIDTH-1:0] fir_tdata;
  logic                                         fir_tvalid;

  fir_inp3_2spc_wrapper fir_inp3_2spc_wrapper_i (
    .aclk               (data_clk_2x),
    .aresetn            (resetn_d2clk),
    .s_axis_data_tdata  (serial_tdata_out),
    .s_axis_data_tvalid (serial_tvalid_out),
    .s_axis_data_tready (),
    .m_axis_data_tdata  (fir_tdata),
    .m_axis_data_tvalid (fir_tvalid)
  );

  //---------------------------------------------------------------------------
  // Round and clip: 35-bit → 16-bit per output path (12 paths)
  //---------------------------------------------------------------------------
  logic [NUM_OUT_PATHS*DATA_WIDTH-1:0] rc_tdata;
  logic [NUM_OUT_PATHS-1:0]            rc_tvalid;

  generate
    for (genvar i = 0; i < NUM_OUT_PATHS; i++) begin : gen_round_clip
      axi_round_and_clip #(
        .WIDTH_IN  (FIR_OUT_WIDTH),
        .WIDTH_OUT (DATA_WIDTH),
        .CLIP_BITS (CLIP_BITS)
      ) axi_round_and_clip_i (
        .clk      (data_clk_2x),
        .reset    (~resetn_d2clk),
        .i_tdata  (fir_tdata[i][FIR_OUT_WIDTH-1:0]),
        .i_tlast  (1'b0),
        .i_tvalid (fir_tvalid),
        .i_tready (),
        .o_tdata  (rc_tdata[i*DATA_WIDTH +: DATA_WIDTH]),
        .o_tlast  (),
        .o_tvalid (rc_tvalid[i]),
        .o_tready (1'b1)
      );
    end
  endgenerate

  //---------------------------------------------------------------------------
  // Gearbox: 6 SPC @ data_clk_2x → 4 SPC @ rfdc_clk
  //---------------------------------------------------------------------------
  dac_gearbox_6x4 dac_gearbox_6x4_i (
    .clk_1x          (data_clk),
    .clk_2x          (data_clk_2x),
    .rf_clk          (rfdc_clk),
    .reset_n_c1      (resetn_dclk),
    .reset_n_c2      (resetn_d2clk),
    .data_c2         (rc_tdata),
    .data_valid_c2   (rc_tvalid[0]),
    .data_rclk       (dac_data_out_tdata),
    .data_valid_rclk (dac_data_out_tvalid)
  );

endmodule : tx_inp3

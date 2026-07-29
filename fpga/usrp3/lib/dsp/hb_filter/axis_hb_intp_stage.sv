//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_intp_stage
//
// Description: Local stage wrapper for one hb interpolation FIR stage.
// This stage accepts two SPC_IN-wide AXI inputs: a direct-path input and an
// upstream previous-stage input. An internal 2:1 mux selects one input stream
// and forwards it to the HB x2 interpolator.
//
// Parameters:
//   SAMP_W: Sample width in bits.
//   SPC_OUT: Samples per cycle output by this stage. Must be a power of 2.
//            The number of input samples per cycle (SPC_IN) is derived as SPC_OUT/2,
//            except when SPC_OUT=1, in which case SPC_IN=1.
//   NUM_COEFFS: Number of FIR coefficients in this stage's HB filter.
//               Can be either 47 or 63. Default is 47.
//   PRELOAD_ZEROES: If 1, the hb interpolation FIR is instantiated with zero preload
//                   enabled. This means the FIR will be flushed upon clear/reset such
//                   that it behaves as after cold power-up with all-zero state.
//
`default_nettype none

module axis_hb_intp_stage #(
  parameter  int SAMP_W         = 48,
  parameter  int SPC_OUT        = 8,
  parameter  int NUM_COEFFS     = axis_hb_utils_pkg::HB47_NUM_COEFFS,
  parameter  bit PRELOAD_ZEROES = 0,
  localparam int SPC_IN = (SPC_OUT > 1) ? (SPC_OUT / 2) : 1
) (
  input  wire                      clk,
  input  wire                      rst,
  input  wire                      clear,
  input  wire                      input_select,
  input  wire [SPC_IN*SAMP_W-1:0]  s_axis_tdata,
  input  wire                      s_axis_tvalid,
  output wire                      s_axis_tready,
  input  wire                      s_axis_tlast,
  input  wire [SPC_IN*SAMP_W-1:0]  s_prev_tdata,
  input  wire                      s_prev_tvalid,
  output wire                      s_prev_tready,
  input  wire                      s_prev_tlast,
  output wire [SPC_OUT*SAMP_W-1:0] m_axis_tdata,
  output wire                      m_axis_tvalid,
  input  wire                      m_axis_tready,
  output wire                      m_axis_tlast
);

  logic [SPC_IN*SAMP_W-1:0] selected_tdata;
  logic                     selected_tvalid;
  logic                     selected_tready;
  logic                     selected_tlast;

  assign selected_tdata  = input_select ? s_prev_tdata : s_axis_tdata;
  assign selected_tvalid = input_select ? s_prev_tvalid : s_axis_tvalid;
  assign selected_tlast  = input_select ? s_prev_tlast : s_axis_tlast;
  assign s_axis_tready   = input_select ? 1'b0 : selected_tready;
  assign s_prev_tready   = input_select ? selected_tready : 1'b0;

  axis_hb_intp_fir #(
    .SAMP_W             (SAMP_W),
    .SPC_IN             (SPC_IN),
    .SPC_OUT            (SPC_OUT),
    .NUM_COEFFS         (NUM_COEFFS),
    .INTERPOLATION_PHASE(1'b0),
    .PRELOAD_ZEROES     (PRELOAD_ZEROES)
  ) hb_intp_fir_x (
    .clk          (clk),
    .rst          (rst),
    .clear        (clear),
    .s_axis_tdata (selected_tdata),
    .s_axis_tvalid(selected_tvalid),
    .s_axis_tlast (selected_tlast),
    .s_axis_tready(selected_tready),
    .m_axis_tdata (m_axis_tdata),
    .m_axis_tvalid(m_axis_tvalid),
    .m_axis_tlast (m_axis_tlast),
    .m_axis_tready(m_axis_tready),
    .enable       (1'b1)
  );

endmodule : axis_hb_intp_stage

`default_nettype wire

//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_decim_stage
//
// Description:
//   Stage wrapper for one halfband decimation FIR stage used inside
//   axis_hb_cascade_decim.
//
//   Wraps one axis_hb_decim_fir instance and exposes two output AXI-S ports:
//     m_direct_*: output toward the width-converter chain (WC chain).
//                 Used when this is the last active decimation stage.
//     m_next_*:   output toward the next stage in the decimation cascade.
//                 Used when further decimation stages follow.
//
//   The output_select port steers the FIR output tready/tvalid to exactly one
//   consumer.  tdata/tlast are always driven from the FIR on both ports so the
//   downstream logic can read data without a mux on the data path:
//     output_select = 0: m_direct_* is active; m_next_tvalid is gated to 0
//     output_select = 1: m_next_*   is active; m_direct_tvalid is gated to 0
//
//   This is the decimation mirror of axis_hb_intp_stage, which selects between
//   two inputs; this module selects between two outputs.
//
// Parameters:
//   SAMP_W:           Sample width in bits.
//   SPC_IN:           Input samples per cycle.
//   NUM_COEFFS:       Number of HB FIR coefficients (47 or 63).
//   DECIMATION_PHASE: Passed through to axis_hb_decim_fir; controls the
//                     temporal decimation phase when SPC_IN=1.
//   PRELOAD_ZEROES:   Passed through to axis_hb_decim_fir.
//   SPC_OUT:          Derived localparam: SPC_IN/2, lower-bounded to 1.
//

`default_nettype none

module axis_hb_decim_stage #(
  parameter  int SAMP_W           = 48,
  parameter  int SPC_IN           = 8,
  parameter  int NUM_COEFFS       = axis_hb_utils_pkg::HB47_NUM_COEFFS,
  parameter  bit DECIMATION_PHASE = 1'b0,
  parameter  bit PRELOAD_ZEROES   = 0,
  localparam int SPC_OUT          = (SPC_IN > 1) ? (SPC_IN / 2) : 1
) (
  input  wire                          clk,
  input  wire                          rst,
  input  wire                          clear,
  // 0 = route FIR output to m_direct_*; 1 = route to m_next_*
  input  wire                          output_select,
  // AXI4-Stream input (from s_axis or previous stage)
  input  wire [SPC_IN-1:0][SAMP_W-1:0] s_axis_tdata,
  input  wire                          s_axis_tvalid,
  output wire                          s_axis_tready,
  input  wire                          s_axis_tlast,
  // Direct output → width-converter chain (active when output_select = 0)
  output wire [SPC_OUT-1:0][SAMP_W-1:0] m_direct_tdata,
  output wire                           m_direct_tvalid,
  input  wire                           m_direct_tready,
  output wire                           m_direct_tlast,
  // Next-stage output → next axis_hb_decim_stage (active when output_select = 1)
  output wire [SPC_OUT-1:0][SAMP_W-1:0] m_next_tdata,
  output wire                           m_next_tvalid,
  input  wire                           m_next_tready,
  output wire                           m_next_tlast
);

  //---------------------------------------------------------------------------
  // Internal FIR signals (flat buses as required by axis_hb_decim_fir)
  //---------------------------------------------------------------------------

  wire [SPC_IN*SAMP_W-1:0]  fir_s_tdata;
  wire [SPC_OUT*SAMP_W-1:0] fir_m_tdata;
  wire                       fir_m_tvalid;
  wire                       fir_m_tready;
  wire                       fir_m_tlast;

  // Cast N-d packed input to flat bus for the FIR
  assign fir_s_tdata = s_axis_tdata;

  // Cast flat FIR output to N-d packed for both output ports (data path, no mux needed)
  assign m_direct_tdata = fir_m_tdata;
  assign m_next_tdata   = fir_m_tdata;

  // Steer tvalid: only one output port is active at a time
  assign m_direct_tvalid = output_select ? 1'b0      : fir_m_tvalid;
  assign m_next_tvalid   = output_select ? fir_m_tvalid : 1'b0;

  // Steer tready back to FIR from the active consumer
  assign fir_m_tready = output_select ? m_next_tready : m_direct_tready;

  // tlast is identical on both ports
  assign m_direct_tlast = fir_m_tlast;
  assign m_next_tlast   = fir_m_tlast;

  //---------------------------------------------------------------------------
  // Halfband decimation FIR instance
  //---------------------------------------------------------------------------

  axis_hb_decim_fir #(
    .SAMP_W          (SAMP_W),
    .SPC_IN          (SPC_IN),
    .SPC_OUT         (SPC_OUT),
    .NUM_COEFFS      (NUM_COEFFS),
    .DECIMATION_PHASE(DECIMATION_PHASE),
    .PRELOAD_ZEROES  (PRELOAD_ZEROES)
  ) hb_decim_fir_x (
    .clk          (clk),
    .rst          (rst),
    .clear        (clear),
    .s_axis_tdata (fir_s_tdata),
    .s_axis_tvalid(s_axis_tvalid),
    .s_axis_tready(s_axis_tready),
    .s_axis_tlast (s_axis_tlast),
    .m_axis_tdata (fir_m_tdata),
    .m_axis_tvalid(fir_m_tvalid),
    .m_axis_tready(fir_m_tready),
    .m_axis_tlast (fir_m_tlast),
    .enable        (1'b1)
  );

endmodule : axis_hb_decim_stage

`default_nettype wire

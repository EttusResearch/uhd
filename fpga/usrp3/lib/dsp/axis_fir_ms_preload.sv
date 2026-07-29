// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_fir_ms_preload
//
// Description:
//   Thin wrapper around axi_fir_multisample_filter that optionally pre-fills
//   the filter pipeline with zeros on reset/clear before allowing real data
//   through such that the filter behaves like after a cold start.
//
//   When PRELOAD_ZEROES=1: on reset or clear, feeds zeroes
//   into the FIR while blocking upstream (s_axis_data_tready=0) and
//   suppressing downstream output (m_axis_data_tvalid=0).
//   After preloading, the wrapper becomes transparent.
//
//   When PRELOAD_ZEROES=0: transparent pass-through; clear is still wired to
//   the FIR.
//
// Parameters:
//   PRELOAD_ZEROES_NUM_TAPS:
//     The number of serial pipeline stages (taps) that axi_fir_multisample_filter
//     actually instantiates after applying its coefficient optimizations.
//     This determines the number of cycles required to flush the FIR pipeline
//     by feeding zeros, and thus how long the input/output should be gated during
//     preload.
//
//     Standard FIR (no optimizations, or RELOADABLE_COEFFS=1, default behavior):
//       PRELOAD_ZEROES_NUM_TAPS = NUM_COEFFS
//       Example: 41-tap FIR -> 41 stages, PIPELINE_DELAY = 46
//
//     Halfband FIR (RELOADABLE_COEFFS=0, symmetric + alternating-zero taps):
//       PRELOAD_ZEROES_NUM_TAPS = (NUM_COEFFS + 5) / 4
//       Example: HBF47 -> 13 stages, PIPELINE_DELAY = 18
//                HBF63 -> 17 stages, PIPELINE_DELAY = 22
//

`default_nettype none

module axis_fir_ms_preload #(
  int IN_WIDTH                 = 16,
  int NUM_SPC                  = 4,
  int COEFF_WIDTH              = 16,
  int OUT_WIDTH                = 16,
  int NUM_COEFFS               = 41,
  int CLIP_BITS                = $clog2(NUM_COEFFS),
  int ACCUM_WIDTH              = IN_WIDTH + COEFF_WIDTH + $clog2(NUM_COEFFS) - 1,
  bit [COEFF_WIDTH-1:0] COEFFS_VEC [NUM_COEFFS] = '{0: (1 << (COEFF_WIDTH-1)) - 1, default: 0},
  bit RELOADABLE_COEFFS        = 1,
  bit BLANK_OUTPUT             = 1,
  bit USE_EMBEDDED_REGS_COEFFS = 1,
  bit PRELOAD_ZEROES           = 0,
  int PRELOAD_ZEROES_NUM_TAPS  = NUM_COEFFS  // See module description above
) (
  input  wire                          clk,
  input  wire                          reset,
  input  wire                          clear,

  input  wire  [NUM_SPC*IN_WIDTH-1:0]  s_axis_data_tdata,
  input  wire                          s_axis_data_tlast,
  input  wire                          s_axis_data_tvalid,
  output logic                         s_axis_data_tready,

  output logic [NUM_SPC*OUT_WIDTH-1:0] m_axis_data_tdata,
  output logic                         m_axis_data_tlast,
  output logic                         m_axis_data_tvalid,
  input  wire                          m_axis_data_tready,

  input  wire  [COEFF_WIDTH-1:0]       s_axis_reload_tdata,
  input  wire                          s_axis_reload_tvalid,
  input  wire                          s_axis_reload_tlast,
  output logic                         s_axis_reload_tready
);

  //----------------------------------------------------------------------------
  // Preload logic
  //----------------------------------------------------------------------------

  // Goal: after preloading, the filter behaves identically to a cold-start.
  // The only observable difference is the PIPELINE_DELAY backpressure cycles on reset/clear.
  //
  // PRELOAD_ZEROES_NUM_TAPS:
  //   number of serial DSP stages in the wrapped FIR
  //   (after potential optimization, see module description).
  //   The FIR's internal PIPELINE_DELAY equals
  //   PRELOAD_ZEROES_NUM_TAPS + 5
  // PIPELINE_FILL_CYCLES:
  //   number of cycles needed to fill FIR pipeline with zeroes.
  // ROUND_CLIP_DELAY:
  //   axi_round_and_clip is not reset by clear and holds 2 in-flight zero beats
  //   after PIPELINE_FILL_CYCLES.
  //   Drain beats drive fir_s_tvalid=0 so they exit through fir_m_tready=1
  //   (suppressed) before the output gate opens, ensuring fir_m_tvalid=0.
  localparam int FIR_DSP_PIPELINE_DELAY = PRELOAD_ZEROES_NUM_TAPS + 5;
  localparam int PIPELINE_FILL_CYCLES   = NUM_COEFFS + FIR_DSP_PIPELINE_DELAY;
  localparam int ROUND_CLIP_DELAY       = 2; // axi_round (1 cycle) + axi_clip (1 cycle)
  localparam int PIPELINE_DELAY         = PIPELINE_FILL_CYCLES + ROUND_CLIP_DELAY;
  localparam int CNT_W                  = $clog2(PIPELINE_DELAY + 1);

  logic [CNT_W-1:0] preload_cnt;
  logic             filling;    // fill phase: drive zeros into FIR with tvalid=1
  logic             preloading; // entire preload window (fill + drain), gate input/output

  always_ff @(posedge clk) begin
    if (reset | clear) begin
      preload_cnt <= '0;
      filling     <= PRELOAD_ZEROES;
      preloading  <= PRELOAD_ZEROES;
    end else if (preloading) begin
      preload_cnt <= preload_cnt + 1'b1;
      filling     <= PRELOAD_ZEROES && (preload_cnt < PIPELINE_FILL_CYCLES - 1);
      preloading  <= PRELOAD_ZEROES && (preload_cnt < PIPELINE_DELAY - 1);
    end
  end

  //----------------------------------------------------------------------------
  // AXI-Stream muxing
  //----------------------------------------------------------------------------

  // Internal signals to/from the wrapped FIR
  logic [NUM_SPC*IN_WIDTH-1:0]  fir_s_tdata;
  logic                         fir_s_tvalid;
  logic                         fir_s_tlast;
  logic                         fir_s_tready;
  logic [NUM_SPC*OUT_WIDTH-1:0] fir_m_tdata;
  logic                         fir_m_tvalid;
  logic                         fir_m_tlast;
  logic                         fir_m_tready;

  // Fill phase   (filling=1): feed zeros with tvalid=1 to fill the FIR shift register.
  // Drain phase  (filling=0, preloading=1): drive tvalid=0 so round+clip empties its
  //              pipeline, ensuring fir_m_tvalid=0 when the output gate opens.
  // Normal phase (preloading=0): fully transparent pass-through.
  // Upstream (s_axis_data_tready) is held low for the entire preload window so no real
  // data is consumed until the filter is clean.
  assign fir_s_tdata        = filling    ? '0   : s_axis_data_tdata;
  assign fir_s_tvalid       = filling    ? 1'b1 : (preloading ? 1'b0 : s_axis_data_tvalid);
  assign fir_s_tlast        = filling    ? 1'b0 : s_axis_data_tlast;
  assign s_axis_data_tready = preloading ? 1'b0 : fir_s_tready;

  // Output gate: hold output suppressed for the entire preload window.
  // fir_m_tready=1 drains round+clip silently during both fill and drain phases.
  // When the gate opens (preloading=0), fir_m_tvalid is guaranteed to be 0.
  assign fir_m_tready       = preloading ? 1'b1 : m_axis_data_tready;
  assign m_axis_data_tdata  = fir_m_tdata;
  assign m_axis_data_tlast  = fir_m_tlast;
  assign m_axis_data_tvalid = preloading ? 1'b0 : fir_m_tvalid;

  //----------------------------------------------------------------------------
  // Wrapped FIR instance
  //----------------------------------------------------------------------------

  axi_fir_multisample_filter #(
    .IN_WIDTH                (IN_WIDTH),
    .NUM_SPC                 (NUM_SPC),
    .COEFF_WIDTH             (COEFF_WIDTH),
    .OUT_WIDTH               (OUT_WIDTH),
    .NUM_COEFFS              (NUM_COEFFS),
    .CLIP_BITS               (CLIP_BITS),
    .ACCUM_WIDTH             (ACCUM_WIDTH),
    .COEFFS_VEC              (COEFFS_VEC),
    .RELOADABLE_COEFFS       (RELOADABLE_COEFFS),
    .BLANK_OUTPUT            (BLANK_OUTPUT),
    .USE_EMBEDDED_REGS_COEFFS(USE_EMBEDDED_REGS_COEFFS)
  ) fir_inst (
    .clk                 (clk),
    .reset               (reset),
    .clear               (clear),
    .s_axis_data_tdata   (fir_s_tdata),
    .s_axis_data_tlast   (fir_s_tlast),
    .s_axis_data_tvalid  (fir_s_tvalid),
    .s_axis_data_tready  (fir_s_tready),
    .m_axis_data_tdata   (fir_m_tdata),
    .m_axis_data_tlast   (fir_m_tlast),
    .m_axis_data_tvalid  (fir_m_tvalid),
    .m_axis_data_tready  (fir_m_tready),
    .s_axis_reload_tdata (s_axis_reload_tdata),
    .s_axis_reload_tvalid(s_axis_reload_tvalid),
    .s_axis_reload_tlast (s_axis_reload_tlast),
    .s_axis_reload_tready(s_axis_reload_tready)
  );

endmodule : axis_fir_ms_preload

`default_nettype wire

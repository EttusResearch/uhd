//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_intp_fir.sv
//
// Description:
//
//  AXI-Stream halfband FIR x2 interpolator for complex I/Q samples.
//
//  Data path:
//    s_axis -> upsamp_x2 -> split I/Q -> axis_fir_ms_preload (I and Q) -> combine I/Q -> m_axis
//
//  The module supports two upsampling operating modes:
//    - Serial mode:   (SPC_IN, SPC_OUT) = (1, 1)
//                     The upsampler emits one original sample
//                     and one inserted zero sample across two cycles.
//    - Parallel mode: SPC_OUT = 2 * SPC_IN
//                     The upsampler inserts zero-valued lanes in the same cycle.
//
//  The module has an enable input that gates the input stream.
//  When enable=0, the module stalls without consuming input data.
//  When enable=1, the module processes data normally.
//
//  Parameter description:
//    - SAMP_W:              Width of one complex sample in bits.
//                           The sample packs I and Q as two SAMP_W/2 halves.
//    - SPC_IN:              Number of complex samples per input clock.
//    - SPC_OUT:             Number of complex samples per output clock after interpolation.
//                           Valid values are (1,1) for serial mode, or 2*SPC_IN for parallel mode.
//    - NUM_COEFFS:          Halfband FIR length. Supported values are 47 and 63.
//    - INTERPOLATION_PHASE: Selects the insertion phase for the zero sample.
//                           In serial mode, 0 emits data then zero, 1 emits zero then data.
//                           In parallel mode, 0 maps each input sample to even lanes and zeros
//                           to odd lanes, while 1 maps zeros to even lanes and data to odd lanes.
//    - PRELOAD_ZEROES:      When 1, pre-fills FIR taps on reset/clear to suppress startup
//                           transients.
//
`default_nettype none

module axis_hb_intp_fir #(
  parameter int SAMP_W              = 32,
  parameter int SPC_IN              = 8,
  parameter int SPC_OUT             = 2 * SPC_IN,
  parameter int NUM_COEFFS          = 47,
  parameter bit INTERPOLATION_PHASE = 1'b0,
  parameter bit PRELOAD_ZEROES      = 1'b1
) (
  // Clock and reset
  input  wire                       clk,
  input  wire                       rst,
  input  wire                       clear,
  // AXI-Stream input interface
  input  wire [SPC_IN*SAMP_W-1:0]   s_axis_tdata,
  input  wire                       s_axis_tvalid,
  input  wire                       s_axis_tlast,
  output logic                      s_axis_tready,
  // AXI-Stream output interface
  output logic [SPC_OUT*SAMP_W-1:0] m_axis_tdata,
  output logic                      m_axis_tvalid,
  output logic                      m_axis_tlast,
  input  wire                       m_axis_tready,
  // Configuration
  input  wire                       enable
);

  import axis_hb_utils_pkg::*;

  localparam int COEFF_WIDTH             = axis_hb_utils_pkg::COEFF_WIDTH;
  localparam int COEFF_FRACTIONAL_BITS   = axis_hb_utils_pkg::COEFF_FRACTIONAL_BITS;
  localparam int NUM_COEFFS_LOG2         = $clog2(NUM_COEFFS);
  localparam int INTERNAL_ACC_WIDTH      = SAMP_W / 2 + COEFF_WIDTH + NUM_COEFFS_LOG2 - 1;
  // For interpolation, the halfband coefficient gain compensates the zero insertion.
  localparam int BITS_TO_ROUND           = COEFF_FRACTIONAL_BITS;
  localparam int BITS_TO_CLIP            = INTERNAL_ACC_WIDTH - (SAMP_W / 2 + BITS_TO_ROUND);
  localparam int PRELOAD_ZEROES_NUM_TAPS = (NUM_COEFFS + 5) / 4;

  // Ensure SPC_IN == SPC_OUT == 1 or SPC_OUT == 2*SPC_IN for valid upsampler configurations.
  if (!(((SPC_IN == 1) && (SPC_OUT == 1)) || (SPC_OUT == (2 * SPC_IN)))) begin : spc_cfg_error
    $error({"Invalid SPC configuration for axis_hb_intp_fir. ",
            "Expected (SPC_IN,SPC_OUT)=(1,1) or SPC_OUT=2*SPC_IN. ",
            "Got SPC_IN=%0d, SPC_OUT=%0d."}, SPC_IN, SPC_OUT);
  end

  // Input gate: s_axis_* re-qualified by enable. When enable=0 the module
  // stalls (valid suppressed, ready deasserted) without consuming input data.
  logic [SPC_IN*SAMP_W-1:0]   src_tdata;
  logic                       src_tvalid;
  logic                       src_tlast;
  logic                       src_tready;

  // Upsampler output
  logic [SPC_OUT*SAMP_W-1:0]  upsamp_tdata;
  logic                       upsamp_tvalid;
  logic                       upsamp_tlast;
  logic                       upsamp_tready;

  // Separated I and Q half-sample streams entering the FIR filters.
  logic [SPC_OUT*SAMP_W/2-1:0] in_tdata_i;
  logic                        in_tvalid_i;
  logic                        in_tready_i;
  logic                        in_tlast_i;
  logic [SPC_OUT*SAMP_W/2-1:0] in_tdata_q;
  logic                        in_tvalid_q;
  logic                        in_tready_q;
  logic                        in_tlast_q;

  // FIR filter outputs for I and Q channels. Both FIRs share identical
  // configuration and receive the same flow control, so they stay in
  // lockstep.
  logic [SPC_OUT*SAMP_W/2-1:0] post_filter_tdata_i;
  logic                        post_filter_tvalid_i;
  logic                        post_filter_tready_i;
  logic                        post_filter_tlast_i;
  logic [SPC_OUT*SAMP_W/2-1:0] post_filter_tdata_q;
  logic                        post_filter_tvalid_q;
  logic                        post_filter_tready_q;
  logic                        post_filter_tlast_q;

  // Recombined I/Q output stream before assignment to m_axis_*.
  logic [SPC_OUT*SAMP_W-1:0] out_tdata;
  logic                      out_tvalid;
  logic                      out_tlast;
  logic                      out_tready;

  assign src_tdata    = s_axis_tdata;
  assign src_tvalid   = s_axis_tvalid && enable;
  assign src_tlast    = s_axis_tlast;
  assign s_axis_tready = enable ? src_tready : 1'b0;

  axis_upsamp_x2 #(
    .SAMP_W             (SAMP_W),
    .SPC_IN             (SPC_IN),
    .SPC_OUT            (SPC_OUT),
    .INTERPOLATION_PHASE(INTERPOLATION_PHASE)
  ) upsamp_x2_x (
    .clk          (clk),
    .rst          (rst),
    .clr          (clear),
    .s_axis_tdata (src_tdata),
    .s_axis_tvalid(src_tvalid),
    .s_axis_tlast (src_tlast),
    .s_axis_tready(src_tready),
    .m_axis_tdata (upsamp_tdata),
    .m_axis_tvalid(upsamp_tvalid),
    .m_axis_tlast (upsamp_tlast),
    .m_axis_tready(upsamp_tready)
  );

  for (genvar samp_idx = 0; samp_idx < SPC_OUT; samp_idx++) begin : split_iq_loop
    assign in_tdata_q[samp_idx*SAMP_W/2 +: SAMP_W/2] = upsamp_tdata[samp_idx*SAMP_W +: SAMP_W/2];
    assign in_tdata_i[samp_idx*SAMP_W/2 +: SAMP_W/2] = upsamp_tdata[samp_idx*SAMP_W + SAMP_W/2 +: SAMP_W/2];
  end : split_iq_loop

  assign in_tvalid_q   = upsamp_tvalid;
  assign in_tvalid_i   = upsamp_tvalid;
  assign in_tlast_q    = upsamp_tlast;
  assign in_tlast_i    = upsamp_tlast;
  assign upsamp_tready = in_tready_q && in_tready_i;

  generate
    if (NUM_COEFFS == 47) begin : generate_hb47
      axis_fir_ms_preload #(
        .IN_WIDTH                (SAMP_W / 2),
        .NUM_SPC                 (SPC_OUT),
        .COEFF_WIDTH             (COEFF_WIDTH),
        .OUT_WIDTH               (SAMP_W / 2),
        .NUM_COEFFS              (NUM_COEFFS),
        .CLIP_BITS               (BITS_TO_CLIP),
        .COEFFS_VEC              (HB47_COEFF_VEC),
        .RELOADABLE_COEFFS       (0),
        .BLANK_OUTPUT            (0),
        .USE_EMBEDDED_REGS_COEFFS(0),
        .PRELOAD_ZEROES          (PRELOAD_ZEROES),
        .PRELOAD_ZEROES_NUM_TAPS (PRELOAD_ZEROES_NUM_TAPS)
      ) hbint_i_x (
        .clk                 (clk),
        .reset               (rst),
        .clear               (clear),
        .s_axis_data_tdata   (in_tdata_i),
        .s_axis_data_tlast   (in_tlast_i),
        .s_axis_data_tvalid  (in_tvalid_i),
        .s_axis_data_tready  (in_tready_i),
        .m_axis_data_tdata   (post_filter_tdata_i),
        .m_axis_data_tlast   (post_filter_tlast_i),
        .m_axis_data_tvalid  (post_filter_tvalid_i),
        .m_axis_data_tready  (post_filter_tready_i),
        .s_axis_reload_tdata ('0),
        .s_axis_reload_tvalid(1'b0),
        .s_axis_reload_tlast (1'b0),
        .s_axis_reload_tready()
      );

      axis_fir_ms_preload #(
        .IN_WIDTH                (SAMP_W / 2),
        .NUM_SPC                 (SPC_OUT),
        .COEFF_WIDTH             (COEFF_WIDTH),
        .OUT_WIDTH               (SAMP_W / 2),
        .NUM_COEFFS              (NUM_COEFFS),
        .CLIP_BITS               (BITS_TO_CLIP),
        .COEFFS_VEC              (HB47_COEFF_VEC),
        .RELOADABLE_COEFFS       (0),
        .BLANK_OUTPUT            (0),
        .USE_EMBEDDED_REGS_COEFFS(0),
        .PRELOAD_ZEROES          (PRELOAD_ZEROES),
        .PRELOAD_ZEROES_NUM_TAPS (PRELOAD_ZEROES_NUM_TAPS)
      ) hbint_q_x (
        .clk                 (clk),
        .reset               (rst),
        .clear               (clear),
        .s_axis_data_tdata   (in_tdata_q),
        .s_axis_data_tlast   (in_tlast_q),
        .s_axis_data_tvalid  (in_tvalid_q),
        .s_axis_data_tready  (in_tready_q),
        .m_axis_data_tdata   (post_filter_tdata_q),
        .m_axis_data_tlast   (post_filter_tlast_q),
        .m_axis_data_tvalid  (post_filter_tvalid_q),
        .m_axis_data_tready  (post_filter_tready_q),
        .s_axis_reload_tdata ('0),
        .s_axis_reload_tvalid(1'b0),
        .s_axis_reload_tlast (1'b0),
        .s_axis_reload_tready()
      );
    end else if (NUM_COEFFS == 63) begin : generate_hb63
      axis_fir_ms_preload #(
        .IN_WIDTH                (SAMP_W / 2),
        .NUM_SPC                 (SPC_OUT),
        .COEFF_WIDTH             (COEFF_WIDTH),
        .OUT_WIDTH               (SAMP_W / 2),
        .NUM_COEFFS              (NUM_COEFFS),
        .CLIP_BITS               (BITS_TO_CLIP),
        .COEFFS_VEC              (HB63_COEFF_VEC),
        .RELOADABLE_COEFFS       (0),
        .BLANK_OUTPUT            (0),
        .USE_EMBEDDED_REGS_COEFFS(0),
        .PRELOAD_ZEROES          (PRELOAD_ZEROES),
        .PRELOAD_ZEROES_NUM_TAPS (PRELOAD_ZEROES_NUM_TAPS)
      ) hbint_i_x (
        .clk                 (clk),
        .reset               (rst),
        .clear               (clear),
        .s_axis_data_tdata   (in_tdata_i),
        .s_axis_data_tlast   (in_tlast_i),
        .s_axis_data_tvalid  (in_tvalid_i),
        .s_axis_data_tready  (in_tready_i),
        .m_axis_data_tdata   (post_filter_tdata_i),
        .m_axis_data_tlast   (post_filter_tlast_i),
        .m_axis_data_tvalid  (post_filter_tvalid_i),
        .m_axis_data_tready  (post_filter_tready_i),
        .s_axis_reload_tdata ('0),
        .s_axis_reload_tvalid(1'b0),
        .s_axis_reload_tlast (1'b0),
        .s_axis_reload_tready()
      );

      axis_fir_ms_preload #(
        .IN_WIDTH                (SAMP_W / 2),
        .NUM_SPC                 (SPC_OUT),
        .COEFF_WIDTH             (COEFF_WIDTH),
        .OUT_WIDTH               (SAMP_W / 2),
        .NUM_COEFFS              (NUM_COEFFS),
        .CLIP_BITS               (BITS_TO_CLIP),
        .COEFFS_VEC              (HB63_COEFF_VEC),
        .RELOADABLE_COEFFS       (0),
        .BLANK_OUTPUT            (0),
        .USE_EMBEDDED_REGS_COEFFS(0),
        .PRELOAD_ZEROES          (PRELOAD_ZEROES),
        .PRELOAD_ZEROES_NUM_TAPS (PRELOAD_ZEROES_NUM_TAPS)
      ) hbint_q_x (
        .clk                 (clk),
        .reset               (rst),
        .clear               (clear),
        .s_axis_data_tdata   (in_tdata_q),
        .s_axis_data_tlast   (in_tlast_q),
        .s_axis_data_tvalid  (in_tvalid_q),
        .s_axis_data_tready  (in_tready_q),
        .m_axis_data_tdata   (post_filter_tdata_q),
        .m_axis_data_tlast   (post_filter_tlast_q),
        .m_axis_data_tvalid  (post_filter_tvalid_q),
        .m_axis_data_tready  (post_filter_tready_q),
        .s_axis_reload_tdata ('0),
        .s_axis_reload_tvalid(1'b0),
        .s_axis_reload_tlast (1'b0),
        .s_axis_reload_tready()
      );
    end else begin : generate_invalid_num_coeffs
      initial begin
        $error("Invalid NUM_COEFFS parameter: %0d. Supported values are 47 and 63.", NUM_COEFFS);
      end
    end
  endgenerate

  // Mux I and Q back into RFNoC format: Q occupies the lower half of each
  // sample word and I occupies the upper half (Q first, then I).
  for (genvar samp_idx = 0; samp_idx < SPC_OUT; samp_idx++) begin : combine_iq_loop
    assign out_tdata[samp_idx*SAMP_W +: SAMP_W/2] =
      post_filter_tdata_q[samp_idx*SAMP_W/2 +: SAMP_W/2];
    assign out_tdata[samp_idx*SAMP_W + SAMP_W/2 +: SAMP_W/2] =
      post_filter_tdata_i[samp_idx*SAMP_W/2 +: SAMP_W/2];
  end : combine_iq_loop

  // I and Q FIRs are in lockstep, so it is sufficient that I-channel signals
  // alone drive the output flow control.
  assign out_tvalid            = post_filter_tvalid_i;
  assign out_tlast             = post_filter_tlast_i;
  assign post_filter_tready_q  = out_tready;
  assign post_filter_tready_i  = out_tready;

  assign m_axis_tdata  = out_tdata;
  assign m_axis_tvalid = out_tvalid;
  assign m_axis_tlast  = out_tlast;
  assign out_tready    = m_axis_tready;

endmodule : axis_hb_intp_fir

`default_nettype wire

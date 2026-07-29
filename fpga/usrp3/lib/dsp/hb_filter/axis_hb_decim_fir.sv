//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_decim_fir.sv
//
// Description:
//
//  AXI4-Stream Halfband Filter Generic FIR Implementation. Splits input
//  samples into I and Q paths, applies halfband FIR filtering to each path,
//  and recombines the output samples. Also decimates the output by a factor of 2.
//

`default_nettype none

module axis_hb_decim_fir #(
  parameter int SAMP_W           = 32,   // Width of each I/Q sample
  parameter int SPC_IN           = 8,    // Number of I/Q samples per clock (input)
  parameter int SPC_OUT          = 4,    // Number of I/Q samples per clock (output)
  parameter int NUM_COEFFS       = 47,   // Number of filter coefficients
  parameter bit DECIMATION_PHASE = 1'b0, // Controls the decimation phase when SPC_IN=1:
                                         //   0 => output starts at sample 0,
                                         //   1 => output starts at sample 1
  parameter bit PRELOAD_ZEROES   = 0     // When 1, pre-fill filter taps with zeros on
                                         // reset/clear to eliminate the startup transient
) (
  // Clock and reset
  input  wire                        clk,
  input  wire                        rst,
  input  wire                        clear,
  // AXI4-Stream input interface
  input  wire  [  SPC_IN*SAMP_W-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,
  input  wire                        s_axis_tlast,
  output logic                       s_axis_tready,
  // AXI4-Stream output interface
  output logic [ SPC_OUT*SAMP_W-1:0] m_axis_tdata,
  output logic                       m_axis_tvalid,
  output logic                       m_axis_tlast,
  input  wire                        m_axis_tready,
  // Configuration
  input  wire                        enable
);
  import axis_hb_utils_pkg::*;
  //-------------------------------------------------------------------------------------------------
  // Default Filter Coefficients
  //-------------------------------------------------------------------------------------------------

  localparam int COEFF_WIDTH           = axis_hb_utils_pkg::COEFF_WIDTH;
  localparam int COEFF_FRACTIONAL_BITS = axis_hb_utils_pkg::COEFF_FRACTIONAL_BITS;
  localparam int COEFF_GAIN_BITS       = axis_hb_utils_pkg::COEFF_GAIN_BITS;
  localparam int NUM_COEFFS_LOG2       = $clog2(NUM_COEFFS);
  localparam int INTERNAL_ACC_WIDTH    = SAMP_W / 2 + COEFF_WIDTH + NUM_COEFFS_LOG2 - 1;
  // BITS_TO_ROUND: LSBs removed by rounding-to-nearest in axi_round_and_clip.
  // Includes COEFF_GAIN_BITS so the rounding stage also compensates for a gain in the
  // filter coefficients if present.
  localparam int BITS_TO_ROUND = COEFF_FRACTIONAL_BITS + COEFF_GAIN_BITS;
  // BITS_TO_CLIP: remaining MSBs removed by saturation (overflow guard bits).
  localparam int BITS_TO_CLIP  = INTERNAL_ACC_WIDTH - (SAMP_W / 2 + BITS_TO_ROUND);
  // For PRELOAD_ZEROES=1, axis_fir_ms_preload requires the number of effective
  // FIR filter slices that will be instantiated for the given filter coefficients.
  // For halfband filters with symmetric coefficients and alternating zero-valued taps, the number of
  // effective taps is (NUM_COEFFS+5)/4 due to symmetry and zero-skipping optimizations in
  // axi_fir_multisample_filter when RELOADABLE_COEFFS=0.
  localparam int PRELOAD_ZEROES_NUM_TAPS = (NUM_COEFFS + 5) / 4;
  //-------------------------------------------------------------------------------------------------
  // Parameter validation
  //-------------------------------------------------------------------------------------------------

  if ((SPC_IN > 1) && (SPC_OUT != (SPC_IN / 2))) begin : spc_mismatch_error
    $error("Invalid SPC_OUT: %0d. Supported value is SPC_IN/2 when SPC_IN > 1.", SPC_OUT);
  end

  if ((SPC_OUT == SPC_IN) && (SPC_IN != 1)) begin : spc_single_error
    $error({"SPC_OUT cannot be equal to SPC_IN unless SPC_IN is 1.",
    " Invalid configuration: SPC_IN=%0d, SPC_OUT=%0d."},
           SPC_IN, SPC_OUT);
  end

  //-------------------------------------------------------------------------------------------------
  // Internal Signals
  //-------------------------------------------------------------------------------------------------

  logic [  SPC_IN*SAMP_W-1:0] post_filter_tdata;
  logic                       post_filter_tvalid;
  logic                       post_filter_tready;
  logic                       post_filter_tlast;

  // Split sample into I and Q paths
  logic [SPC_IN*SAMP_W/2-1:0] in_tdata_i;
  logic                       in_tvalid_i;
  logic                       in_tready_i;
  logic                       in_tlast_i;
  logic [SPC_IN*SAMP_W/2-1:0] post_filter_tdata_i;
  logic                       post_filter_tvalid_i;
  logic                       post_filter_tready_i;
  logic                       post_filter_tlast_i;
  logic [SPC_IN*SAMP_W/2-1:0] in_tdata_q;
  logic                       in_tvalid_q;
  logic                       in_tready_q;
  logic                       in_tlast_q;
  logic [SPC_IN*SAMP_W/2-1:0] post_filter_tdata_q;
  logic                       post_filter_tvalid_q;
  logic                       post_filter_tready_q;
  logic                       post_filter_tlast_q;
  logic [ SPC_OUT*SAMP_W-1:0] out_tdata;
  logic                       out_tvalid;
  logic                       out_tlast;
  logic                       out_tready;

  //-------------------------------------------------------------------------------------------------
  // FIR Filter Instances for I and Q paths
  //-------------------------------------------------------------------------------------------------

  for (genvar samp_idx = 0; samp_idx < SPC_IN; samp_idx++) begin : split_iq_loop
    assign in_tdata_i[samp_idx*SAMP_W/2+:SAMP_W/2] = s_axis_tdata[samp_idx*SAMP_W+:SAMP_W/2];
    assign in_tdata_q[samp_idx*SAMP_W/2+:SAMP_W/2] = s_axis_tdata[samp_idx*SAMP_W+SAMP_W/2+:SAMP_W/2];
  end
  assign in_tvalid_i = s_axis_tvalid;
  assign in_tvalid_q = s_axis_tvalid;
  assign s_axis_tready = in_tready_i && in_tready_q;  // Assume both paths ready at same time
  assign in_tlast_i = s_axis_tlast;
  assign in_tlast_q = s_axis_tlast;

  // Generic FIR implementation for halfband filter 1
  // Can process SPC input samples and outputs SPC output samples
  // therefore needing no additional buffering between stages.
  generate
    if (NUM_COEFFS == 47) begin : generate_hb47
      axis_fir_ms_preload #(
          .IN_WIDTH(SAMP_W / 2),
          .NUM_SPC(SPC_IN),
          .COEFF_WIDTH(COEFF_WIDTH),
          .OUT_WIDTH(SAMP_W / 2),
          .NUM_COEFFS(NUM_COEFFS),
          .CLIP_BITS(BITS_TO_CLIP),
          .COEFFS_VEC(HB47_COEFF_VEC),
          .RELOADABLE_COEFFS(0),
          .BLANK_OUTPUT(0),
          .USE_EMBEDDED_REGS_COEFFS(0),
          .PRELOAD_ZEROES(PRELOAD_ZEROES),
          .PRELOAD_ZEROES_NUM_TAPS(PRELOAD_ZEROES_NUM_TAPS)
      ) hbdec_i (
          .clk                 (clk),
          .reset               (rst),
          .clear               (clear),
          // Input stream
          .s_axis_data_tdata   (in_tdata_i),
          .s_axis_data_tlast   (in_tlast_i),
          .s_axis_data_tvalid  (in_tvalid_i && enable),
          .s_axis_data_tready  (in_tready_i),
          // Output stream
          .m_axis_data_tdata   (post_filter_tdata_i),
          .m_axis_data_tlast   (post_filter_tlast_i),
          .m_axis_data_tvalid  (post_filter_tvalid_i),
          .m_axis_data_tready  (post_filter_tready_i),
          // Reload Interface (Unused)
          .s_axis_reload_tdata ('0),
          .s_axis_reload_tvalid(1'b0),
          .s_axis_reload_tlast (1'b0),
          .s_axis_reload_tready()
      );

      // Generic FIR implementation for halfband filter 1
      // Can process SPC input samples and outputs SPC output samples
      // therefore needing no additional buffering between stages.
      axis_fir_ms_preload #(
          .IN_WIDTH(SAMP_W / 2),
          .NUM_SPC(SPC_IN),
          .COEFF_WIDTH(COEFF_WIDTH),
          .OUT_WIDTH(SAMP_W / 2),
          .NUM_COEFFS(NUM_COEFFS),
          .CLIP_BITS(BITS_TO_CLIP),
          .COEFFS_VEC(HB47_COEFF_VEC),
          .RELOADABLE_COEFFS(0),
          .BLANK_OUTPUT(0),
          .USE_EMBEDDED_REGS_COEFFS(0),
          .PRELOAD_ZEROES(PRELOAD_ZEROES),
          .PRELOAD_ZEROES_NUM_TAPS(PRELOAD_ZEROES_NUM_TAPS)
      ) hbdec_q (
          .clk(clk),
          .reset(rst),
          .clear(clear),
          // Input stream
          .s_axis_data_tdata(in_tdata_q),
          .s_axis_data_tlast(in_tlast_q),
          .s_axis_data_tvalid(in_tvalid_q && enable),
          .s_axis_data_tready(in_tready_q),
          // Output stream
          .m_axis_data_tdata(post_filter_tdata_q),
          .m_axis_data_tlast(post_filter_tlast_q),
          .m_axis_data_tvalid(post_filter_tvalid_q),
          .m_axis_data_tready(post_filter_tready_q),
          // Reload Interface (Unused)
          .s_axis_reload_tdata('0),
          .s_axis_reload_tvalid(1'b0),
          .s_axis_reload_tlast(1'b0),
          .s_axis_reload_tready()
      );
    end else if (NUM_COEFFS == 63) begin : generate_hb63
      axis_fir_ms_preload #(
          .IN_WIDTH(SAMP_W / 2),
          .NUM_SPC(SPC_IN),
          .COEFF_WIDTH(COEFF_WIDTH),
          .OUT_WIDTH(SAMP_W / 2),
          .NUM_COEFFS(NUM_COEFFS),
          .CLIP_BITS(BITS_TO_CLIP),
          .COEFFS_VEC(HB63_COEFF_VEC),
          .RELOADABLE_COEFFS(0),
          .BLANK_OUTPUT(0),
          .USE_EMBEDDED_REGS_COEFFS(0),
          .PRELOAD_ZEROES(PRELOAD_ZEROES),
          .PRELOAD_ZEROES_NUM_TAPS(PRELOAD_ZEROES_NUM_TAPS)
      ) hbdec_i (
          .clk                 (clk),
          .reset               (rst),
          .clear               (clear),
          // Input stream
          .s_axis_data_tdata   (in_tdata_i),
          .s_axis_data_tlast   (in_tlast_i),
          .s_axis_data_tvalid  (in_tvalid_i && enable),
          .s_axis_data_tready  (in_tready_i),
          // Output stream
          .m_axis_data_tdata   (post_filter_tdata_i),
          .m_axis_data_tlast   (post_filter_tlast_i),
          .m_axis_data_tvalid  (post_filter_tvalid_i),
          .m_axis_data_tready  (post_filter_tready_i),
          // Reload Interface (Unused)
          .s_axis_reload_tdata ('0),
          .s_axis_reload_tvalid(1'b0),
          .s_axis_reload_tlast (1'b0),
          .s_axis_reload_tready()
      );

      // Generic FIR implementation for halfband filter 1
      // Can process SPC input samples and outputs SPC output samples
      // therefore needing no additional buffering between stages.
      axis_fir_ms_preload #(
          .IN_WIDTH(SAMP_W / 2),
          .NUM_SPC(SPC_IN),
          .COEFF_WIDTH(COEFF_WIDTH),
          .OUT_WIDTH(SAMP_W / 2),
          .NUM_COEFFS(NUM_COEFFS),
          .CLIP_BITS(BITS_TO_CLIP),
          .COEFFS_VEC(HB63_COEFF_VEC),
          .RELOADABLE_COEFFS(0),
          .BLANK_OUTPUT(0),
          .USE_EMBEDDED_REGS_COEFFS(0),
          .PRELOAD_ZEROES(PRELOAD_ZEROES),
          .PRELOAD_ZEROES_NUM_TAPS(PRELOAD_ZEROES_NUM_TAPS)
      ) hbdec_q (
          .clk(clk),
          .reset(rst),
          .clear(clear),
          // Input stream
          .s_axis_data_tdata(in_tdata_q),
          .s_axis_data_tlast(in_tlast_q),
          .s_axis_data_tvalid(in_tvalid_q && enable),
          .s_axis_data_tready(in_tready_q),
          // Output stream
          .m_axis_data_tdata(post_filter_tdata_q),
          .m_axis_data_tlast(post_filter_tlast_q),
          .m_axis_data_tvalid(post_filter_tvalid_q),
          .m_axis_data_tready(post_filter_tready_q),
          // Reload Interface (Unused)
          .s_axis_reload_tdata('0),
          .s_axis_reload_tvalid(1'b0),
          .s_axis_reload_tlast(1'b0),
          .s_axis_reload_tready()
      );

    end else begin : generate_invalid_num_coeffs
      initial begin
        $error("Invalid NUM_COEFFS parameter: %0d. Supported values are 47 and 63.", NUM_COEFFS);
        $fatal;
      end
    end
  endgenerate
  // Recombine I and Q paths
  for (genvar samp_idx = 0; samp_idx < SPC_IN; samp_idx++) begin : combine_iq_loop
    assign post_filter_tdata[samp_idx*SAMP_W +: SAMP_W/2] =
      post_filter_tdata_i[samp_idx*SAMP_W/2 +: SAMP_W/2];
    assign post_filter_tdata[samp_idx*SAMP_W + SAMP_W/2 +: SAMP_W/2] =
      post_filter_tdata_q[samp_idx*SAMP_W/2 +: SAMP_W/2];
  end

  // Prioritize AXI HS signals from I path. Assume they are aligned.
  assign post_filter_tvalid = post_filter_tvalid_i;
  assign post_filter_tlast = post_filter_tlast_i;
  assign post_filter_tready_i = post_filter_tready;
  assign post_filter_tready_q = post_filter_tready;

  // Decimation logic to reduce samples by 2
  if ((SPC_IN == 1) && (SPC_OUT == 1)) begin : decim_over_time
    // For single sample per clock, decimation is achieved by taking every 2nd sample in time
    logic toggle_sample;
    logic pair_phase;
    logic delayed_tlast;
    logic tlast_fifo_i_data, tlast_fifo_i_valid;
    logic tlast_fifo_o_data, tlast_fifo_o_valid, tlast_fifo_o_ready;

    always_ff @(posedge clk) begin
      if (rst | clear | in_tlast_i) begin
        pair_phase <= 1'b0;
        delayed_tlast <= 1'b0;
      end else if (in_tvalid_i && enable && in_tready_i) begin
        pair_phase <= ~pair_phase;
        delayed_tlast <= in_tlast_i;
      end
    end

    // Store one tlast bit for each pair of input samples.
    assign tlast_fifo_i_valid = in_tvalid_i && enable && in_tready_i && (pair_phase | in_tlast_i);
    assign tlast_fifo_i_data = delayed_tlast | in_tlast_i;
    assign tlast_fifo_o_ready = out_tvalid && out_tready;

    axi_fifo #(
      .WIDTH(1),
      .SIZE (1)
    ) tlast_fifo (
      .clk      (clk),
      .reset    (rst),
      .clear    (clear),
      .i_tdata  (tlast_fifo_i_data),
      .i_tvalid (tlast_fifo_i_valid),
      .i_tready (),
      .o_tdata  (tlast_fifo_o_data),
      .o_tvalid (tlast_fifo_o_valid),
      .o_tready (tlast_fifo_o_ready),
      .space    (),
      .occupied ()
    );

    always_ff @(posedge clk) begin
      if (rst | clear) begin
        toggle_sample <= ~DECIMATION_PHASE;
      end else if (post_filter_tvalid && post_filter_tready) begin
        toggle_sample <= ~toggle_sample;
      end
    end
    assign out_tdata  = post_filter_tdata;                   // output the current sample directly
    assign out_tvalid = post_filter_tvalid && toggle_sample && tlast_fifo_o_valid;
    assign out_tlast  = tlast_fifo_o_valid && tlast_fifo_o_data;
    assign post_filter_tready = toggle_sample
      ? (out_tready && tlast_fifo_o_valid) : 1'b1;

  end else begin : decim_by_2
    // For multiple samples per clock, decimation is achieved by taking every 2nd sample in the vector
    for (genvar samp_idx = 0; samp_idx < SPC_OUT; samp_idx++) begin : decim_loop
      assign out_tdata[samp_idx*SAMP_W+:SAMP_W] = post_filter_tdata[(2*samp_idx)*SAMP_W+:SAMP_W];
    end
    assign out_tvalid         = post_filter_tvalid;
    assign out_tlast          = post_filter_tlast;
    assign post_filter_tready = out_tready;
  end

  //-------------------------------------------------------------------------------------------------
  // Output Assignments
  //-------------------------------------------------------------------------------------------------
  assign m_axis_tdata       = out_tdata;
  assign m_axis_tvalid      = out_tvalid;
  assign m_axis_tlast       = out_tlast;
  assign out_tready         = m_axis_tready;

endmodule : axis_hb_decim_fir

`default_nettype wire

//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: vector_iir
// Description: 
//   This module implements an IIR filter with a variable length delay line.
//   Transfer Function:                 beta
//                        H(z) = ------------------
//                               1 - alpha*z^-delay
//   where:
//   - beta is the feedforward tap
//   - alpha is the feedback tap
//   - delay (aka vector_len) is the feedback tap delay
//
// Parameters:
//   - MAX_VECTOR_LEN: Maximum value for delay (vector_len)
//   - IN_W: Input sample width for a real sample which includes the sign bit.
//           The actual input of the module will be 2*IN_W because it handles
//           complex data.
//   - OUT_W: Output sample width for a real sample which includes the sign bit.
//           The actual output of the module will be 2*OUT_W because it handles
//           complex data.
//   - ALPHA_W: Width of the alpha parameter (signed)
//   - BETA_W: Width of the beta parameter (signed)
//   - FEEDBACK_W: Number of bits in the feedback delay line (optimal = 25)
//   - ACCUM_HEADROOM: Number of bits of headroom in the feedback accumulator
// Signals:
//   - i_*  : Input sample stream (AXI-Stream)
//   - o_*  : Output sample stream (AXI-Stream)
//   - set_*: Static settings
//

module vector_iir #(
  parameter MAX_VECTOR_LEN  = 1023,
  parameter IN_W            = 16,
  parameter OUT_W           = 16,
  parameter ALPHA_W         = 16,
  parameter BETA_W          = 16,
  parameter FEEDBACK_W      = 25,
  parameter ACCUM_HEADROOM  = 4
)(
  input  wire                                clk,
  input  wire                                reset,
  input  wire [$clog2(MAX_VECTOR_LEN+1)-1:0] set_vector_len,
  input  wire [BETA_W-1:0]                   set_beta,
  input  wire [ALPHA_W-1:0]                  set_alpha,
  input  wire [IN_W*2-1:0]                   i_tdata,
  input  wire                                i_tlast,
  input  wire                                i_tvalid,
  output wire                                i_tready,
  output wire [OUT_W*2-1:0]                  o_tdata,
  output wire                                o_tlast,
  output wire                                o_tvalid,
  input  wire                                o_tready
);

  // There are four registers between the input and output
  // - Input pipeline (in_X_reg)
  // - Feedforward product (ff_prod_X_reg)
  // - Feedback sum (fb_sum_X_reg)
  // - Output pipeline (dsp_data_out)
  localparam IN_TO_OUT_LATENCY = 4;

  // The feedback path has 3 cycles of delay
  // - Feedback sum (fb_sum_X_reg)
  // - variable_delay_line (2 cyc)
  // - Scaled feedback (fb_sum_scaled_X_reg)
  localparam MIN_FB_DELAY = 4;

  // Pipeline settings for timing
  reg        [$clog2(MAX_VECTOR_LEN-MIN_FB_DELAY)-1:0] reg_fb_delay;
  reg signed [                             BETA_W-1:0] reg_beta;
  reg signed [                            ALPHA_W-1:0] reg_alpha;

  always @(posedge clk) begin
    reg_fb_delay <= set_vector_len - MIN_FB_DELAY - 1;  //Adjust for pipeline delay
    reg_beta     <= set_beta;
    reg_alpha    <= set_alpha;
  end

  //-----------------------------------------------------------
  // AXI-Stream wrapper
  //-----------------------------------------------------------
  wire [(IN_W*2)-1:0]           dsp_data_in;
  reg  [(OUT_W*2)-1:0]          dsp_data_out = 0;
  wire [IN_TO_OUT_LATENCY-1:0]  chain_en;

  // We are implementing an N-cycle DSP operation without AXI-Stream handshaking.
  // Use an axis_shift_register and the associated strobes to drive clock enables
  // on the DSP regs to ensure that data/valid/last sync up.
  axis_shift_register #(
    .WIDTH(IN_W*2), .NSPC(1), .LATENCY(IN_TO_OUT_LATENCY),
    .SIDEBAND_DATAPATH(1), .PIPELINE("NONE")
  ) axis_shreg_i (
    .clk(clk), .reset(reset),
    .s_axis_tdata(i_tdata), .s_axis_tkeep(1'b1), .s_axis_tlast(i_tlast),
    .s_axis_tvalid(i_tvalid), .s_axis_tready(i_tready),
    .m_axis_tdata(o_tdata), .m_axis_tkeep(), .m_axis_tlast(o_tlast),
    .m_axis_tvalid(o_tvalid), .m_axis_tready(o_tready),
    .stage_stb(chain_en), .stage_eop(),
    .m_sideband_data(dsp_data_in), .m_sideband_keep(),
    .s_sideband_data(dsp_data_out)
  );

  //-----------------------------------------------------------
  // DSP datapath
  //-----------------------------------------------------------
  localparam FF_PROD_W = IN_W + BETA_W - 1;
  localparam FB_PROD_W = FEEDBACK_W + ALPHA_W - 1;

  reg  signed [IN_W-1:0]        in_i_reg = 0, in_q_reg = 0;
  reg  signed [FF_PROD_W-1:0]   ff_prod_i_reg = 0, ff_prod_q_reg = 0;
  reg  signed [FB_PROD_W-1:0]   fb_sum_i_reg = 0, fb_sum_q_reg = 0;
  wire signed [FB_PROD_W-1:0]   fb_trunc_i, fb_trunc_q;
  wire signed [FEEDBACK_W-1:0]  fb_sum_del_i, fb_sum_del_q;
  reg  signed [FB_PROD_W-1:0]   fb_sum_scaled_i_reg = 0, fb_sum_scaled_q_reg = 0;
  wire signed [OUT_W-1:0]       out_i_rnd, out_q_rnd;

  always @(posedge clk) begin
    if (reset) begin
      {in_i_reg, in_q_reg} <= 0;
      ff_prod_i_reg <= 0;
      ff_prod_q_reg <= 0;
      fb_sum_i_reg <= 0;
      fb_sum_q_reg <= 0;
      fb_sum_scaled_i_reg <= 0;
      fb_sum_scaled_q_reg <= 0;
      dsp_data_out <= 0;
    end else begin
      if (chain_en[0]) begin
        // Input pipeline register
        {in_i_reg, in_q_reg} <= dsp_data_in;
      end
      if (chain_en[1]) begin
        // Feedforward product (x[n] * beta)
        ff_prod_i_reg <= in_i_reg * reg_beta;
        ff_prod_q_reg <= in_q_reg * reg_beta;
        // Compute scaled, delayed feedback (y[n-D] * alpha)
        fb_sum_scaled_i_reg <= fb_sum_del_i * reg_alpha;
        fb_sum_scaled_q_reg <= fb_sum_del_q * reg_alpha;
      end
      if (chain_en[2]) begin
        // Sum of feedforward product and scaled, delayed feedback
        // y[n] = (alpha * y[n-D]) + (x[n] * beta)
        fb_sum_i_reg <= fb_sum_scaled_i_reg + (ff_prod_i_reg <<< (FB_PROD_W - FF_PROD_W - ACCUM_HEADROOM));
        fb_sum_q_reg <= fb_sum_scaled_q_reg + (ff_prod_q_reg <<< (FB_PROD_W - FF_PROD_W - ACCUM_HEADROOM));
      end
      if (chain_en[3]) begin
        // Output pipeline register
        dsp_data_out <= {out_i_rnd, out_q_rnd};
      end
    end
  end

  // Truncate feedback to the requested FEEDBACK_W
  assign fb_trunc_i = (fb_sum_i_reg >>> (FB_PROD_W - FEEDBACK_W));
  assign fb_trunc_q = (fb_sum_q_reg >>> (FB_PROD_W - FEEDBACK_W));

  // A variable delay line will be used to store the feedback
  // This delay line stores "reg_fb_delay" worth of samples which
  // allows each element in the vector to have it's own independent state
  variable_delay_line #(
    .WIDTH(FEEDBACK_W * 2), .DEPTH(MAX_VECTOR_LEN - MIN_FB_DELAY),
    .DYNAMIC_DELAY(1), .DEFAULT_DATA(0), .OUT_REG(1)
  ) delay_line_inst (
    .clk(clk), .clk_en(chain_en[1]), .reset(reset),
    .stb_in(1'b1),
    .data_in({fb_trunc_i[FEEDBACK_W-1:0], fb_trunc_q[FEEDBACK_W-1:0]}),
    .delay(reg_fb_delay),
    .data_out({fb_sum_del_i, fb_sum_del_q})
  );

  // Round the accumulator output to produce the final output
  round #(
    .bits_in(FB_PROD_W-ACCUM_HEADROOM), .bits_out(OUT_W)
  ) out_round_i_inst (
    .in(fb_sum_i_reg[FB_PROD_W-ACCUM_HEADROOM-1:0]), .out(out_i_rnd), .err()
  );
  round #(
    .bits_in(FB_PROD_W-ACCUM_HEADROOM), .bits_out(OUT_W)
  ) out_round_q_inst (
    .in(fb_sum_q_reg[FB_PROD_W-ACCUM_HEADROOM-1:0]), .out(out_q_rnd), .err()
  );

endmodule // vector_iir

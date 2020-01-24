//
// Copyright 2017 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Multiply-accumulate with preadder for use as a computation unit
// in FIR filters. Designed to infer a DSP48 for all registers
// and arithmetic.
//
// Parameters:
//   IN_WIDTH                 - Input width
//   COEFF_WIDTH              - Coefficient width
//   ACCUM_WIDTH              - Accumulator width
//   OUT_WIDTH                - Output width
//
module fir_filter_slice #(
  parameter IN_WIDTH     = 16,
  parameter COEFF_WIDTH  = 16,
  parameter ACCUM_WIDTH  = 32,
  parameter OUT_WIDTH    = 32)
(
  input clk,
  input reset,
  input clear,
  input sample_in_stb,
  input signed [IN_WIDTH-1:0] sample_in_a,       // Sample in
  input signed [IN_WIDTH-1:0] sample_in_b,       // Sample in for symmetric filters
  output signed [IN_WIDTH-1:0] sample_forward,   // Delayed sample in to forward
  input signed [COEFF_WIDTH-1:0] coeff_in,       // Filter tap coefficient
  output signed [COEFF_WIDTH-1:0] coeff_forward, // Filter tap coefficient to forward
  input coeff_load_stb,                          // Load coefficient
  input signed [ACCUM_WIDTH-1:0] sample_accum,   // Accumulating path
  output signed [OUT_WIDTH-1:0] sample_out       // Result
);

  reg signed [IN_WIDTH-1:0] a_reg[0:1];
  reg signed [IN_WIDTH-1:0] d_reg;
  reg signed [IN_WIDTH:0] ad_reg;
  reg signed [COEFF_WIDTH-1:0] b_reg[0:1];
  reg signed [IN_WIDTH+COEFF_WIDTH:0] m_reg;
  reg signed [ACCUM_WIDTH-1:0] p_reg;

  always @(posedge clk) begin
    if (reset | clear) begin
      a_reg[0]   <= 0;
      a_reg[1]   <= 0;
      d_reg      <= 0;
      b_reg[0]   <= 0;
      b_reg[1]   <= 0;
      ad_reg     <= 0;
      m_reg      <= 0;
      p_reg      <= 0;
    end else begin
      if (sample_in_stb) begin
        a_reg[0] <= sample_in_a;
        a_reg[1] <= a_reg[0];
        d_reg    <= sample_in_b;
        ad_reg   <= a_reg[1] + d_reg;
        m_reg    <= ad_reg * b_reg[1];
        p_reg    <= sample_accum + m_reg;
      end
      if (coeff_load_stb) begin
        b_reg[0] <= coeff_in;
      end
      b_reg[1]   <= b_reg[0];
    end
  end

  assign coeff_forward  = b_reg[0];
  assign sample_forward = a_reg[1];
  assign sample_out     = p_reg[OUT_WIDTH-1:0];

endmodule

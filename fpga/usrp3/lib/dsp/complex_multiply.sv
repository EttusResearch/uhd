//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: complex_multiply
//
// Description:
//
//  This module performs a complex multiplication of two complex numbers. The
//  module has a fixed latency of 4 clock cycles in which the enable signal
//  needs to be asserted. The input widths are configurable but are limited by
//  the DSP48 block of the Xilinx FPGA, which is the target for synthesis. The
//  multiplication result is returned with full precision of WIDTH_A + WIDTH_B +
//  1 bits, which is the worst case for the output width.
//
// Parameters:
//
//   WIDTH_A : Width of the real and imaginary parts of the first complex
//             number. The maximum width is 25 bits.
//   WIDTH_B : Width of the real and imaginary parts of the second complex
//             number. The maximum width is 18 bits.
//

module complex_multiply #(
  int WIDTH_A = 25,
  int WIDTH_B = 18,
  localparam WIDTH_OUT = WIDTH_A + WIDTH_B + 1
)(
  input  logic clk,
  input  logic enable,

  input  logic signed [WIDTH_A-1 : 0] a_real,
  input  logic signed [WIDTH_A-1 : 0] a_imag,
  input  logic signed [WIDTH_B-1 : 0] b_real,
  input  logic signed [WIDTH_B-1 : 0] b_imag,

  output logic signed [WIDTH_OUT-1 : 0] out_real,
  output logic signed [WIDTH_OUT-1 : 0] out_imag
);

  // try to reuse the registers from DSP48 block of the Xilinx FPGA
  logic signed [WIDTH_A-1:0] a_real_reg, a_imag_reg, a2_imag_reg;
  logic signed [WIDTH_B-1:0] b_real_reg, b_imag_reg, b2_real_reg, b2_imag_reg;
  logic signed [WIDTH_OUT-1:0] mult_real_1, mult_real_2,
    mult_imag_1, mult_imag_2, mult_real_1_d, mult_imag_1_d;

  // check widths to comply with the Xilinx DSP48 block
  if (WIDTH_A > 25)
    $error("Input width A exceeds 25 bits, which is the maximum for DSP48.");
  if (WIDTH_B > 18)
    $error("Input width B exceeds 18 bits, which is the maximum for DSP48.");

  // calculate (a+bi) * (c+di) = (ac-bd) + (ad+bc)i
  always_ff @(posedge clk) begin
    if (enable) begin
      // cycle 1 - register the inputs
      a_real_reg <= a_real;
      a_imag_reg <= a_imag;
      b_real_reg <= b_real;
      b_imag_reg <= b_imag;

      // cycle 2 - calculate the first summands and delay inputs for the second
      // summands
      mult_real_1 <= a_real_reg * b_real_reg;
      mult_imag_1 <= a_real_reg * b_imag_reg;

      a2_imag_reg <= a_imag_reg;
      b2_real_reg <= b_real_reg;
      b2_imag_reg <= b_imag_reg;

      // cycle 3 - calculate the second multiplications and delay the existing
      // products
      mult_real_2 <= a2_imag_reg * b2_imag_reg;
      mult_imag_2 <= a2_imag_reg * b2_real_reg;

      mult_real_1_d <= mult_real_1;
      mult_imag_1_d <= mult_imag_1;

      // cycle 4 - sum the results
      out_real <= mult_real_1_d - mult_real_2;
      out_imag <= mult_imag_1_d + mult_imag_2;
    end
  end

endmodule

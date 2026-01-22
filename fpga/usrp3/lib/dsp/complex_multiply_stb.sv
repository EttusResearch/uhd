//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: complex_multiply_stb
//
// Description:
//
//  This module performs a complex multiplication of two complex numbers. The
//  complex numbers are given in the form of two inputs. Each input contains
//  the real and imaginary parts concatenated, with the real part in the lower
//  bits. The width of the real and imaginary parts is given by the DATA_WIDTH
//  parameter of the input, which must be a multiple of 2. The number of
//  fractional bits for each input can be configured using the respective
//  parameters.
//
//  This version uses strobe/enable interface without fall through,instead of
//  AXI streams. The strobe signal must be asserted for the data to progess
//  through the module.
//  The total latency of this module is 6 clock cycles.
//
//  This module uses the RFNoC data item definition for complex data, where the upper bits
//  of the sample represent the real part and the lower bits represent the imaginary part.
//  See "RFNoC specification - Data Item and Component Ordering" for details.
//
// Parameters:
//
//   WIDTH_A : Width of the real/imag components of the first complex
//              operand. Maximum width is 25 bits.
//   WIDTH_B : Width of the real/imag components of the second complex
//              operand. Maximum width is 18 bits.
//   WIDTH_PRODUCT : Width of the real/imag components of the product.
//   FRACTIONAL_BITS_A : Number of fractional bits for the first complex number.
//   FRACTIONAL_BITS_B : Number of fractional bits for the second complex number.
//   FRACTIONAL_BITS_PRODUCT : Number of fractional bits for the product.
//

module complex_multiply_stb #(
    parameter int WIDTH_A = 20,
    parameter int WIDTH_B = 16,
    parameter int WIDTH_PRODUCT = 20,
    parameter int FRACTIONAL_BITS_A = 0,
    parameter int FRACTIONAL_BITS_B = 0,
    parameter int FRACTIONAL_BITS_PRODUCT = 0
) (
    input  logic                       clk,
    input  logic                       rst,
    input  logic                       strobe_in,
    input  logic [      2*WIDTH_A-1:0] factor_a,
    input  logic [      2*WIDTH_B-1:0] factor_b,
    output logic [2*WIDTH_PRODUCT-1:0] product,
    output logic                       strobe_out
);
  // local parameters
  localparam int MODULE_LATENCY = 6;

  // Check the input parameters
  if (FRACTIONAL_BITS_A < 0) $error("FRACTIONAL_BITS_A must be positive.");
  if (FRACTIONAL_BITS_B < 0) $error("FRACTIONAL_BITS_B must be positive.");
  if (FRACTIONAL_BITS_PRODUCT < 0) $error("FRACTIONAL_BITS_PRODUCT must be positive.");
  // must be less as for signed information there must be at least one bit for the sign
  if (FRACTIONAL_BITS_A >= WIDTH_A)
    $error("FRACTIONAL_BITS_A must be less than DATA_WIDTH of factor_a.");
  if (FRACTIONAL_BITS_B >= WIDTH_B)
    $error("FRACTIONAL_BITS_B must be less than DATA_WIDTH of factor_b.");
  if (FRACTIONAL_BITS_PRODUCT > FRACTIONAL_BITS_A + FRACTIONAL_BITS_B)
    $error(
        "FRACTIONAL_BITS_PRODUCT must be less than or equal to the sum",
        " of FRACTIONAL_BITS_A and FRACTIONAL_BITS_B."
    );

  // Split the input into real and imaginary parts
  logic signed [WIDTH_A-1 : 0] a_real, a_imag;
  logic signed [WIDTH_B-1 : 0] b_real, b_imag;
  always_comb begin
    a_real = factor_a[WIDTH_A+:WIDTH_A];
    a_imag = factor_a[      0+:WIDTH_A];
    b_real = factor_b[WIDTH_B+:WIDTH_B];
    b_imag = factor_b[      0+:WIDTH_B];
  end

  // Width of the resulting multiplication output
  localparam int MULT_OUTPUT_WIDTH = WIDTH_A + WIDTH_B + 1;
  // full precision output of the multiplier
  logic signed [2*MULT_OUTPUT_WIDTH-1:0] product_full;

  // number crunching
  complex_multiply #(
      .WIDTH_A(WIDTH_A),
      .WIDTH_B(WIDTH_B)
  ) complex_multiply_i (
      .clk(clk),
      .enable(strobe_in),
      .a_real(a_real),
      .a_imag(a_imag),
      .b_real(b_real),
      .b_imag(b_imag),
      .out_real(product_full[0+:MULT_OUTPUT_WIDTH]),
      .out_imag(product_full[MULT_OUTPUT_WIDTH+:MULT_OUTPUT_WIDTH])
  );

  // Determine how many bits need to be clipped in order to conform to the
  // desired output width and fractional bits.
  // If the number is > 0, then rounding and clipping is needed.
  // Calculation is based on two parts:
  //   - Difference in raw bit width between multiplication output and desired output:
  //     - MULT_OUTPUT_WIDTH - WIDTH_PRODUCT
  //   - Difference in fractional bits between multiplication output and desired output:
  //     - FRACTIONAL_BITS_PRODUCT - (FRACTIONAL_BITS_A + FRACTIONAL_BITS_B)
  localparam int CLIP_BITS = (MULT_OUTPUT_WIDTH - WIDTH_PRODUCT +
    FRACTIONAL_BITS_PRODUCT - FRACTIONAL_BITS_A - FRACTIONAL_BITS_B);
  if (CLIP_BITS < 0)
    $error("The number of output bits to saturate has to be a positive value or 0.");

  // Sign extend if rounding and clipping is needed
  localparam int WIDTH_ROUND_IN = (CLIP_BITS) ? MULT_OUTPUT_WIDTH + 1 : MULT_OUTPUT_WIDTH;
  localparam int WIDTH_ROUND_OUT = (CLIP_BITS) ? WIDTH_PRODUCT + CLIP_BITS + 1 : MULT_OUTPUT_WIDTH;

  // split complex
  logic signed [WIDTH_ROUND_IN-1:0] product_full_real, product_full_imag;

  assign product_full_real = (CLIP_BITS) ?
    {product_full[MULT_OUTPUT_WIDTH-1], product_full[0+:MULT_OUTPUT_WIDTH]} :
    product_full[0+:MULT_OUTPUT_WIDTH];
  assign product_full_imag = (CLIP_BITS) ?
    {product_full[(2*MULT_OUTPUT_WIDTH)-1], product_full[MULT_OUTPUT_WIDTH+:MULT_OUTPUT_WIDTH]} :
    product_full[MULT_OUTPUT_WIDTH+:MULT_OUTPUT_WIDTH];

  // rounding
  logic signed [WIDTH_ROUND_OUT-1:0] round_out_real, round_out_imag;
  logic signed [WIDTH_ROUND_OUT-1:0] product_rounded_real, product_rounded_imag;

  round #(
      .bits_in (WIDTH_ROUND_IN),
      .bits_out(WIDTH_ROUND_OUT)
  ) round_real (
      .in (product_full_real),
      .out(round_out_real),
      .err()
  );
  round #(
      .bits_in (WIDTH_ROUND_IN),
      .bits_out(WIDTH_ROUND_OUT)
  ) round_imag (
      .in (product_full_imag),
      .out(round_out_imag),
      .err()
  );
  always_ff @(posedge clk) begin : assign_rounded
    if (strobe_in) begin
      product_rounded_real <= round_out_real;
      product_rounded_imag <= round_out_imag;
    end
  end

  // clipping
  logic signed [WIDTH_PRODUCT-1:0] clip_out_real, clip_out_imag;
  logic signed [WIDTH_PRODUCT-1:0] product_clipped_real, product_clipped_imag;
  clip #(
      .bits_in (WIDTH_ROUND_OUT),
      .bits_out(WIDTH_PRODUCT)
  ) clip_real (
      .in (product_rounded_real),
      .out(clip_out_real)
  );
  clip #(
      .bits_in (WIDTH_ROUND_OUT),
      .bits_out(WIDTH_PRODUCT)
  ) clip_imag (
      .in (product_rounded_imag),
      .out(clip_out_imag)
  );
  always_ff @(posedge clk) begin : assign_clipped
    if (strobe_in) begin
      product_clipped_real <= clip_out_real;
      product_clipped_imag <= clip_out_imag;
    end
  end
  //combine complex
  assign product = {product_clipped_real, product_clipped_imag};

  // Strobe signal handling
  // We need to delay the strobe signal by MODULE_LATENCY cycles to match the
  // output data timing.
  logic [MODULE_LATENCY-1:0] strobe_shift_reg;
  always_ff @(posedge clk) begin : strobe_delay
    if (strobe_in) begin
      strobe_shift_reg <= {strobe_shift_reg[MODULE_LATENCY-2:0], strobe_in};
    end
    if (rst) begin
      strobe_shift_reg <= '0;
    end
  end
  assign strobe_out = strobe_shift_reg[MODULE_LATENCY-1] & strobe_in;


endmodule

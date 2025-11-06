//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: complex_multiply_iq
//
// Description:
//
//  This module performs a complex multiplication of two complex numbers. The
//  complex numbers are given in the form of two streams. Each stream's tdata
//  contains the real and imaginary parts concatenated, with the real part in
//  the lower bits. The width of the real and imaginary parts is given by the
//  DATA_WIDTH parameter of the stream, which must be a multiple of 2. The
//  number of fractional bits for each input can be configured using the
//  respective parameters.
//
//  The tlast signal of the first input stream (factor_a) is propagated to the
//  output stream (product). The AXI signals tuser and tkeep are not used in
//  this module.
//
// Parameters:
//
//   FRACTIONAL_BITS_A : Number of fractional bits for the first complex number.
//   FRACTIONAL_BITS_B : Number of fractional bits for the second complex number.
//   FRACTIONAL_BITS_PRODUCT : Number of fractional bits for the product.
//

module complex_multiply_iq #(
  int FRACTIONAL_BITS_A = 0,
  int FRACTIONAL_BITS_B = 0,
  int FRACTIONAL_BITS_PRODUCT = 0
)(
  AxiStreamIf.slave factor_a,
  AxiStreamIf.slave factor_b,
  AxiStreamIf.master product
);

  // Check the input parameters
  if ((factor_a.DATA_WIDTH % 2) != 0)
    $error("DATA_WIDTH of factor_a must be a multiple of 2.");
  if ((factor_b.DATA_WIDTH % 2) != 0)
    $error("DATA_WIDTH of factor_b must be a multiple of 2.");
  if ((product.DATA_WIDTH % 2) != 0)
    $error("DATA_WIDTH of product must be a multiple of 2.");
  if (FRACTIONAL_BITS_A < 0)
    $error("FRACTIONAL_BITS_A must be positive.");
  if (FRACTIONAL_BITS_B < 0)
    $error("FRACTIONAL_BITS_B must be positive.");
  if (FRACTIONAL_BITS_PRODUCT < 0)
    $error("FRACTIONAL_BITS_PRODUCT must be positive.");
  // must be less as for signed information there must be at least one bit for the sign
  if (FRACTIONAL_BITS_A >= factor_a.DATA_WIDTH)
    $error("FRACTIONAL_BITS_A must be less than DATA_WIDTH of factor_a.");
  if (FRACTIONAL_BITS_B >= factor_b.DATA_WIDTH)
    $error("FRACTIONAL_BITS_B must be less than DATA_WIDTH of factor_b.");
  if (FRACTIONAL_BITS_PRODUCT > FRACTIONAL_BITS_A + FRACTIONAL_BITS_B)
    $error("FRACTIONAL_BITS_PRODUCT must be less than or equal to the sum",
    " of FRACTIONAL_BITS_A and FRACTIONAL_BITS_B.");

  // Extract local parameters
  localparam int WIDTH_A = factor_a.DATA_WIDTH / 2;
  localparam int WIDTH_B = factor_b.DATA_WIDTH / 2;
  localparam int WIDTH_PRODUCT = product.DATA_WIDTH / 2;

  // Split the input into real and imaginary parts
  logic signed [WIDTH_A-1 : 0] a_real, a_imag;
  logic signed [WIDTH_B-1 : 0] b_real, b_imag;
  always_comb begin
    a_imag = factor_a.tdata[2*WIDTH_A-1 : WIDTH_A];
    a_real = factor_a.tdata[WIDTH_A-1 : 0];
    b_imag = factor_b.tdata[2*WIDTH_B-1 : WIDTH_B];
    b_real = factor_b.tdata[WIDTH_B-1 : 0];
  end

  // do a complex multiplication
  localparam MULT_OUTPUT_WIDTH = WIDTH_A + WIDTH_B + 1;
  AxiStreamIf #(
    .DATA_WIDTH(MULT_OUTPUT_WIDTH * 2),
    .TUSER(0),
    .TKEEP(0)
  ) product_full (
    .clk(product.clk),
    .rst(product.rst)
  );

  // number crunching
  complex_multiply #(
    .WIDTH_A(WIDTH_A),
    .WIDTH_B(WIDTH_B)
  ) complex_multiply_iq_i (
    .clk(product.clk),
    .enable(product_full.tready),
    .a_real(a_real),
    .a_imag(a_imag),
    .b_real(b_real),
    .b_imag(b_imag),
    .out_real(product_full.tdata[0 +: MULT_OUTPUT_WIDTH]),
    .out_imag(product_full.tdata[MULT_OUTPUT_WIDTH +: MULT_OUTPUT_WIDTH])
  );

  // shift register to compensate for the latency of the multiplication
  localparam MULTIPLY_LATENCY = 4;
  logic [MULTIPLY_LATENCY-1:0] valid_shift_reg = '0;
  logic [MULTIPLY_LATENCY-1:0] last_shift_reg = '0;
  always_ff @(posedge product.clk) begin
    if (product.rst) begin
      valid_shift_reg <= '0;
      last_shift_reg <= '0;
    end else if (product_full.tready) begin
      // shift the valid and last signals
      valid_shift_reg <= {valid_shift_reg[MULTIPLY_LATENCY-2:0], factor_a.tvalid & factor_b.tvalid};
      last_shift_reg <= {last_shift_reg[MULTIPLY_LATENCY-2:0], factor_a.tlast};
    end
  end

  // assign control signals for the multiplier output
  always_comb begin
    product_full.tlast = last_shift_reg[MULTIPLY_LATENCY-1];
    product_full.tvalid = valid_shift_reg[MULTIPLY_LATENCY-1];
    factor_a.tready = product_full.tready & factor_a.tvalid & factor_b.tvalid;
    factor_b.tready = product_full.tready & factor_a.tvalid & factor_b.tvalid;
  end

  // scale down to the original width
  // Cut down the growth of the multiplication result in the first line
  // and compensate for the fractional bits.
  localparam CLIP_BITS = MULT_OUTPUT_WIDTH - WIDTH_PRODUCT
    - FRACTIONAL_BITS_A - FRACTIONAL_BITS_B + FRACTIONAL_BITS_PRODUCT;
  if (CLIP_BITS < 0)
    $error("The number of output bits to saturate has to be a positive value or 0.");

  // Use a single flop to comply with the AXI Stream protocol when the output
  // width is the same as the product. No rounding or clipping is needed in this
  // case.
  if (CLIP_BITS == 0 && MULT_OUTPUT_WIDTH == WIDTH_PRODUCT) begin
    axi_fifo_flop2 #(
      .WIDTH(product.DATA_WIDTH + 1)
    ) out_flop (
      .clk     (product.clk),
      .reset   (product.rst),
      .clear   ('0),
      .i_tdata ({product_full.tdata, product_full.tlast}),
      .i_tvalid(product_full.tvalid),
      .i_tready(product_full.tready),
      .o_tdata ({product.tdata, product.tlast}),
      .o_tvalid(product.tvalid),
      .o_tready(product.tready),
      .space   (),
      .occupied()
    );
  end else begin
    axi_round_and_clip_complex #(
    .WIDTH_IN (MULT_OUTPUT_WIDTH),
    .WIDTH_OUT(WIDTH_PRODUCT),
    .CLIP_BITS(CLIP_BITS),
    .FIFOSIZE (0)
  ) axi_round_and_clipx (
    .clk   (product.clk),
    .reset   (product.rst),
    .i_tdata (product_full.tdata),
    .i_tlast (product_full.tlast),
    .i_tvalid(product_full.tvalid),
    .i_tready(product_full.tready),
    .o_tdata (product.tdata),
    .o_tlast (product.tlast),
    .o_tvalid(product.tvalid),
    .o_tready(product.tready)
  );
  end

endmodule

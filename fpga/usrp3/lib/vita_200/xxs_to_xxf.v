//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//

`define log2(N) ( N < 2    ? 0 : \
                  N < 4    ? 1 : \
                  N < 8    ? 2 : \
                  N < 16   ? 3 : \
                  N < 32   ? 4 : \
                  N < 64   ? 5 : \
                  N < 128  ? 6 : \
                  N < 256  ? 7 : \
                  N < 512  ? 8 : \
                  N < 1024 ? 9 : \
                             10 \
                )

module xxs_to_xxf
#(
  parameter FBITS = 32,               // # of bits for the float
  parameter integer QWIDTH = 16       // # of bits in total, e.g. 16 for a Q15
)
(
  input  [QWIDTH-1:0] i_fixed,
  output [FBITS-1:0]  o_float
);

  // # of bits for the mantissa
  parameter MBITS = 23;

  // # of bits for the exponent
  parameter integer EBITS = 8;

  // # of fractional bits, e.g. 15 for Q15
  parameter integer RADIX = 15;

  // the bias for the exponent
  parameter integer BIAS = (1 << EBITS -1) - 1;

  // the padding for the mantissa
  parameter         PADDING = {(MBITS-QWIDTH){1'b0}};

  // Check for sign
  wire              is_neg   = i_fixed[QWIDTH-1];
  // Check for zero
  wire              is_zero  = i_fixed == 'h0;
  // Get absolute value
  wire [QWIDTH-1:0] abs      = is_neg ? ~i_fixed + 1 : i_fixed;

  wire [`log2(QWIDTH)-1:0] leading_zero;

  wire [QWIDTH-1:0] shift = QWIDTH-leading_zero;

  wire [QWIDTH-1:0] abs_shifted = abs << shift;

  wire [MBITS-1:0]  mantissa;
  wire [EBITS-1:0]  exponent;

  // Determine the position of the leading zero
  // using priority encoding.
  priority_encoder #
  (
    .WIDTH(QWIDTH)
  )
  pe0
  (
    .in(abs),
    .out(leading_zero)
  );

  // This was only tested for the case MBITS > QWIDTH
  generate
  if (MBITS > QWIDTH) begin
    assign mantissa = is_zero ? 23'h0 : {abs_shifted, PADDING};
  end
  else begin
    assign mantissa = abs_shifted[QWIDTH-1:QWIDTH-MBITS];
  end
  endgenerate

  assign exponent = is_zero ? 8'h0 : BIAS + QWIDTH - RADIX - shift;

  assign o_float = {is_neg, exponent, mantissa};
endmodule

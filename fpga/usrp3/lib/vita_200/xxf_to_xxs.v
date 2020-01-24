//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module xxf_to_xxs
#(
  parameter FBITS = 32,               // # of bits for the float
  parameter integer QWIDTH = 16       // # of bits in total, e.g. 16 for a Q15
)
(
  input      [FBITS-1:0]   i_float,
  output reg [QWIDTH-1:0]  o_fixed
);

  // # of bits for the mantissa
  parameter MBITS = 23;

  // # of bits for the exponent
  parameter integer EBITS = 8;

  // # of fractional bits, e.g. 15 for Q15
  parameter integer RADIX = 15;

  // the bias for the exponent
  parameter integer BIAS = (1 << EBITS -1) - 1;

  // the min/max values displayable in Qx.x format
  parameter integer MIN = 1 << RADIX;
  parameter integer MAX = (1 << RADIX) - 1;

  parameter integer SDIFF = RADIX - MBITS - BIAS;

  // Dissect the IEEE 754 float
  wire             is_neg   = i_float[FBITS-1];
  wire [EBITS-1:0] exponent = i_float[MBITS+EBITS-1:MBITS];
  wire [MBITS-1:0] mantissa = i_float[MBITS-1:0];

  // check for +/- zero
  wire is_zero    = (exponent == 'h0) && (mantissa == 'h0);

  // check for normal / denormalized
  wire is_denorm  = (exponent == 'h0) && (mantissa != 'h0);
  wire is_norm    = !is_denorm;

  // check for infty TODO: parametrize!
  wire is_inf     = (exponent == 'hff) && (mantissa == 'h0);

  // check for NaN TODO: parametrize!
  wire is_nan     = (exponent == 'hff) && (mantissa != 'h0);

  // calculate shift
  wire signed [EBITS-1:0] shift = $signed(SDIFF[EBITS-1:0])
                                + $signed(exponent);

  wire [FBITS-1:0] shifted_mant = (shift < 0) ?
                                  {1'b1, mantissa} >> -shift
                                : {1'b1, mantissa} >> -shift;

  // if shifted value cannot be displayed by Q15 numbers, truncate to MAX/MIN
  wire [QWIDTH-1:0] sat_mant    = (shifted_mant > 16'h8000 && is_neg) ?
                                  MIN
                                : (shifted_mant > 16'h7fff && !is_neg) ?
                                  MAX : shifted_mant;

  always @(*) begin
    if (is_inf)
      o_fixed = (is_neg) ? MIN : MAX;
    else if (is_denorm || is_zero)
      o_fixed = 16'h0;
    else begin
      o_fixed = (is_neg) ? ~sat_mant + 1 : sat_mant;
    end
  end
endmodule

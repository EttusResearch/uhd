//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: complex_multiply_iq_all_tb
//
// Description:
//
//  Wrapper for multiple instances of complex_multiply_iq_tb.
//

module complex_multiply_iq_all_tb;
  import PkgMath::*;

  // max value of parameters, full precision, no fractional bits
  complex_multiply_iq_tb #(
    .WIDTH_A(25),
    .WIDTH_B(18),
    .WIDTH_PRODUCT(25+18+1),
    .FRACTIONAL_BITS_A(0),
    .FRACTIONAL_BITS_B(0),
    .FRACTIONAL_BITS_PRODUCT(0)
  ) tb_0 ();

  // typical use case to multiply 16 bit IQ with factor in range [-1, 1)
  complex_multiply_iq_tb #(
    .WIDTH_A(24),
    .WIDTH_B(16),
    .WIDTH_PRODUCT(16),
    .FRACTIONAL_BITS_A(22),
    .FRACTIONAL_BITS_B(0),
    .FRACTIONAL_BITS_PRODUCT(0)
  ) tb_1 ();

  // small numbers
  complex_multiply_iq_tb #(
    .WIDTH_A(4),
    .WIDTH_B(7),
    .WIDTH_PRODUCT(10),
    .FRACTIONAL_BITS_A(2),
    .FRACTIONAL_BITS_B(6),
    .FRACTIONAL_BITS_PRODUCT(7)
  ) tb_2 ();

  // factor a with only fractional bits
  complex_multiply_iq_tb #(
    .WIDTH_A(10),
    .WIDTH_B(10),
    .WIDTH_PRODUCT(15),
    .FRACTIONAL_BITS_A(12),
    .FRACTIONAL_BITS_B(4),
    .FRACTIONAL_BITS_PRODUCT(10)
  ) tb_3 ();

endmodule

//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: PkgMath
//
// Description:
//
//   SystemVerilog supports many Math functions. This adds a few that it
//   doesn't have built in, as well as useful mathematical constants.
//
//   SystemVerilog has built-in support for the following:
//
//     $clog2   $asin
//     $ln      $acos
//     $log10   $atan
//     $exp     $atan2
//     $sqrt    $hypot
//     $pow     $sinh
//     $floor   $cosh
//     $ceil    $tanh
//     $sin     $asinh
//     $cos     $acosh
//     $tan     $atanh
//

package PkgMath;

  //---------------------------------------------------------------------------
  // Constants
  //---------------------------------------------------------------------------

  localparam real PI  = 2*$acos(0.0);
  localparam real TAU = 4*$acos(0.0);

  localparam real PHI = (1 + $sqrt(5.0)) / 2.0;
  localparam real E   = $exp(1);

  localparam byte     BYTE_MAX  =  8'sh7F;
  localparam byte     BYTE_MIN  =  8'sh80;
  localparam shortint SHORT_MAX = 16'sh7FFF;
  localparam shortint SHORT_MIN = 16'sh8000;
  localparam int      INT_MAX   = 32'sh7FFFFFFF;
  localparam int      INT_MIN   = 32'sh80000000;
  localparam longint  LONG_MAX  = 64'sh7FFFFFFFFFFFFFFF;
  localparam longint  LONG_MIN  = 64'sh8000000000000000;

  localparam byte     unsigned UBYTE_MAX  =  8'hFF;
  localparam byte     unsigned UBYTE_MIN  =  8'h00;
  localparam shortint unsigned USHORT_MAX = 16'hFFFF;
  localparam shortint unsigned USHORT_MIN = 16'h0000;
  localparam int      unsigned UINT_MAX   = 32'hFFFFFFFF;
  localparam int      unsigned UINT_MIN   = 32'h00000000;
  localparam longint  unsigned ULONG_MAX  = 64'hFFFFFFFFFFFFFFFF;
  localparam longint  unsigned ULONG_MIN  = 64'h0000000000000000;


  //---------------------------------------------------------------------------
  // Functions (For real data types)
  //---------------------------------------------------------------------------

  // Return the absolute value
  function automatic real abs(real num);
    if (num < 0) return -1.0*num;
    return num;
  endfunction : abs

  // Round a float to the nearest whole number, rounding away from zero for 0.5
  // (same as C++ and default SystemVerilog behavior).
  function automatic real round(real num);
    if (num >= 0) begin
      // Round toward +inf
      if (num - $floor(num) < 0.5) return $floor(num);
      return $ceil(num);
    end else begin
      // Round toward -inf
      if (num - $floor(num) <= 0.5) return $floor(num);
      return $ceil(num);
    end

  endfunction : round

  // Round a float to the nearest value having bits to the right of the binary
  // point. For example:
  //
  //   1.2265625 (0b1.0011101) --> 3 bits --> 1.25000 (0b1.0100000)
  //   1.2265625 (0b1.0011101) --> 5 bits --> 1.21875 (0b1.0011100)
  //
  function automatic real round_bits(real num, int unsigned bits);
    return round(num * 2.0**bits) / (2.0**bits);
  endfunction : round_bits

  // Return the sign of num as +1.0 or -1.0;
  function automatic real sign(real num);
    if (num < 0.0) return -1.0;
    return 1.0;
  endfunction : sign;

  // Return the modulus (remainder) of a / b, with the sign of the numerator.
  // This should match the C++ standard library std::fmod() behavior, as well
  // as SystemVerilog % operator with integer values.
  function automatic real fmod(real a, real b);
    a = abs(a);
    b = abs(b);
    return sign(b) * (a - ($floor(a / b) * b));
  endfunction : fmod

  // Return the (remainder) of a / b, where the quotient is rounded to the
  // nearest integer. This should approximate the C++ standard library
  // std::remainder() behavior.
  function automatic real remainder(real a, real b);
    return a - round(a/b)*b;
  endfunction : remainder

  // Return the maximum of a and b.
  function automatic real fmax(real a, real b);
    if (a > b) return a;
    return b;
  endfunction : fmax

  // Return the minimum of a and b.
  function automatic real fmin(real a, real b);
    if (a < b) return a;
    return b;
  endfunction : fmin


  //---------------------------------------------------------------------------
  // Template Functions (For any data type)
  //---------------------------------------------------------------------------

  class Math #(type T);

    static function T abs(T num);
      if (num < 0) return -num;
      return num;
    endfunction : abs

    static function T sign(T num);
      if (num < 0) return -1;
      return 1;
    endfunction : sign

    static function T max(T a, T b);
      if (a > b) return a;
      else return b;
    endfunction : max

    static function T min(T a, T b);
      if (a < b) return a;
      else return b;
    endfunction : min

  endclass : Math

endpackage : PkgMath

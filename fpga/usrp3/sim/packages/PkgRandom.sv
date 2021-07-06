//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: PkgRandom
//
// Description:
//
//   SystemVerilog has great randomization support, but some features require a
//   more expensive license or aren't supported by all tools. This package
//   tries to fill that gap by providing some useful randomization functions
//   beyond what's supported by standard Verilog.
//

package PkgRandom;

  import PkgMath::*;

  //---------------------------------------------------------------------------
  // Functions
  //---------------------------------------------------------------------------

  // Return a real value in the range [0,max), where max is 1.0 by default.
  function automatic real frand(real max = 1.0);
    bit [63:0] real_bits;
    real num;

    // Build a double-precision floating point value per IEEE-754 standard,
    // which SystemVerilog follows.

    // Positive, with exponent 0
    real_bits[63:52] = 12'h3FF;

    // Mantissa in the range [1.0, 2.0). The leading 1 in the mantissa is
    // implied by the floating point format.
    real_bits[31: 0] = $urandom();
    real_bits[51:32] = $urandom();

    // Compensate for the implied leading 1 in the mantissa by subtracting 1.
    num = $bitstoreal(real_bits) - 1.0;

    // Scale the result to return a value in the desired range.
    return num * max;
  endfunction : frand


  // Return a real value in the range [a,b), [b,a), or [0,a) depending on
  // whether a or b is larger and whether b is provided. This matches the
  // behavior of $urandom_range().
  //
  //   frand_range(1.0, 2.0) -> Random value in the range [1,2)
  //   frand_range(2.0, 1.0) -> Random value in the range [1,2)
  //   frand_range(1.0)      -> Random value in the range [0,1)
  //
  function automatic real frand_range(real a = 1.0, real b = 0.0);
    if (a > b) return b + frand(a - b);
    if (b > a) return a + frand(b - a);
    return a;
  endfunction : frand_range


  // Return a real value with a normal distribution, having the mean value mu
  // and standard deviation sigma.
  function automatic real frandn(real sigma = 1.0, real mu = 0.0);
    // Use the Box-Muller transform to convert uniform random variables to a
    // Gaussian one.
    return sigma*$sqrt(-2.0*$ln(frand())) * $cos(TAU*frand()) + mu;
  endfunction : frandn


  //---------------------------------------------------------------------------
  // Template Functions
  //---------------------------------------------------------------------------

  class Rand #(WIDTH = 64);

    // These are static class functions. They can be called directly, as in:
    //
    //   Rand#(N)::rand_bit()
    //
    // Or, you can declare an object reference, as in:
    //
    //   Rand #(N) rand;
    //   rand.rand_bit();

    typedef bit        [WIDTH-1:0] unsigned_t;
    typedef bit signed [WIDTH-1:0] signed_t;


    // Returns a WIDTH-bit random bit packed array.
    static function unsigned_t rand_bit();
      unsigned_t result;
      int num_rand32 = (WIDTH + 31) / 32;
      for (int i = 0; i < num_rand32; i++) begin
        result = {result, $urandom()};
      end
      return result;
    endfunction : rand_bit


    // Returns a WIDTH-bit random number in the UNSIGNED range [a,b], [b,a], or
    // [0,a] depending on whether a or b is greater and if b is provided. This
    // is equivalent to $urandom_range() but works with any length.
    static function bit [WIDTH-1:0] rand_bit_range(
      unsigned_t a = {WIDTH{1'b1}},
      unsigned_t b = 0
    );
      unsigned_t num;
      int num_bits;
      if (a > b) begin
        // Swap a and b
        unsigned_t temp;
        temp = a;
        a = b;
        b = temp;
      end
      num_bits = $clog2(b - a + unsigned_t'{1});
      do begin
        num = a + (rand_bit() & ((unsigned_t'{1} << num_bits) - 1));
      end while (num > b);
      return num;
    endfunction : rand_bit_range


    // Returns a random number in the given SIGNED range. Behavior is the same
    // as rand_bit_range(), bunsigned_t treats the range values as SIGNED numbers.
    static function signed_t rand_sbit_range(
      signed_t a = {1'b0, {WIDTH{1'b1}}},
      signed_t b = 0
    );
      if (a > b) return b + $signed(rand_bit_range(0, a-b));
      if (b > a) return a + $signed(rand_bit_range(0, b-a));
      return a;
    endfunction : rand_sbit_range


    // Rand#(WIDTH)::rand_logic() returns a WIDTH-bit random logic packed
    // array. Each bit will be 0 or 1 with equal probability (not X or Z).
    static function logic [WIDTH-1:0] rand_logic();
      return rand_bit();
    endfunction : rand_logic

  endclass : Rand

endpackage : PkgRandom

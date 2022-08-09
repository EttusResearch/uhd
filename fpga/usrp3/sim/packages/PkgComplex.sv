//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: PkgComplex
//
// Description:
//
//   A package for doing complex arithmetic in SystemVerilog simulations.
//   Fixed-point operations are implemented such that results clip to the range
//   [-1.0, 1.0) and are rounded to the nearest ULP (half ULP is rounded away
//   from zero, following SystemVerilog rounding behavior).
//


package PkgComplex;

  //---------------------------------------------------------------------------
  // Type Definitions
  //---------------------------------------------------------------------------

  // Define a signed 16-bit fixed-point type with 15 fractional bits (Q0.15).
  typedef bit signed [15:0] s16_t;

  // Signed complex 16-bit data type, the standard type used by UHD and RFNoC.
  typedef struct packed {
    s16_t re;
    s16_t im;
  } sc16_t;

  // Complex floating point data type.
  typedef struct {
    real re;
    real im;
  } complex_t;

  // Maximum and minimum allowed by the s16 type.
  localparam s16_t MAX_S16 = 16'h7FFF;
  localparam s16_t MIN_S16 = 16'h8000;


  //---------------------------------------------------------------------------
  // Conversion Functions
  //---------------------------------------------------------------------------

  // Create an sc16 value from two s16 values.
  function sc16_t build_sc16(s16_t x = 0, s16_t y = 0);
    sc16_t val;
    val.re = x;
    val.im = y;
    return val;
  endfunction : build_sc16

  // Create a complex value from two real values.
  function complex_t build_complex(real x = 0.0, real y = 0.0);
    complex_t val;
    val.re = x;
    val.im = y;
    return val;
  endfunction : build_complex

  // Convert s16 to real.
  function real s16_to_real(s16_t x);
    return real'(x) / (2.0**15);
  endfunction : s16_to_real

  // Convert real to s16.
  function s16_t real_to_s16(real x);
    real val;
    val = x * (2.0**15);
    val = (val > MAX_S16) ? MAX_S16 : val;
    val = (val < MIN_S16) ? MIN_S16 : val;
    return s16_t'(val);
  endfunction : real_to_s16

  // Convert complex to sc16.
  function sc16_t complex_to_sc16(complex_t x);
    sc16_t val;
    val.re = real_to_s16(x.re);
    val.im = real_to_s16(x.im);
    return val;
  endfunction : complex_to_sc16

  // Convert sc16 to complex.
  function complex_t sc16_to_complex(sc16_t x);
    complex_t val;
    val.re = s16_to_real(x.re);
    val.im = s16_to_real(x.im);
    return val;
  endfunction : sc16_to_complex

  // Convert polar coordinates to a complex number. The phase should be in
  // radians.
  function complex_t polar_to_complex(real mag, real phase);
    complex_t val;
    val.re = mag * $cos(phase);
    val.im = mag * $sin(phase);
    return val;
  endfunction : polar_to_complex

  // Convert polar coordinates to an sc16 complex number. The phase should be
  // in radians.
  function sc16_t polar_to_sc16(real mag, real phase);
    return complex_to_sc16(polar_to_complex(mag, phase));
  endfunction : polar_to_sc16


  //---------------------------------------------------------------------------
  // Floating Point Complex Arithmetic
  //---------------------------------------------------------------------------

  // Add two complex numbers: x + y
  function complex_t add(complex_t x, complex_t y);
    complex_t val;
    val.re = x.re + y.re;
    val.im = x.im + y.im;
    return val;
  endfunction : add

  // Subtract two complex numbers: x - y
  function complex_t sub(complex_t x, complex_t y);
    complex_t val;
    val.re = x.re - y.re;
    val.im = x.im - y.im;
    return val;
  endfunction : sub

  // Multiply two complex numbers: x * y
  function complex_t mul(complex_t x, complex_t y);
    complex_t val;
    val.re = x.re*y.re - x.im*y.im;
    val.im = x.re*y.im + x.im*y.re;
    return val;
  endfunction : mul

  // Divide two complex numbers: x / y
  function complex_t div(complex_t x, complex_t y);
    complex_t z;
    z.re = (x.re*y.re + x.im*y.im) / (y.re*y.re + y.im*y.im);
    z.im = (x.im*y.re + x.re*y.im) / (y.re*y.re + y.im*y.im);
    return z;
  endfunction : div

  // Compute the exponential: e^x
  function complex_t exp(complex_t x);
    complex_t val;
    // exp(a+jb) = exp(a)*exp(jb) = exp(a)*(cos(b) + j*sin(b))
    val.re = $exp(x.re)*$cos(x.im);
    val.im = $exp(x.re)*$sin(x.im);
    return val;
  endfunction : exp

  // Compute the sine: sin(x)
  function complex_t sin(complex_t x);
    complex_t val;
    val.re = $sin(x.re)*$cosh(x.im);
    val.im = $cos(x.re)*$sinh(x.im);
    return val;
  endfunction : sin

  // Compute the cosine: cos(x)
  function complex_t cos(complex_t x);
    complex_t val;
    val.re =      $cos(x.re)*$cosh(x.im);
    val.im = -1.0*$sin(x.re)*$sinh(x.im);
    return val;
  endfunction : cos

  // Compute the magnitude/modulus/absolute value: |x|
  function real mag(complex_t x);
    return $sqrt(x.re*x.re + x.im*x.im);
  endfunction : mag

  // Compute the phase/argument: arg(x)
  function real arg(complex_t x);
    return $atan2(x.im, x.re);
  endfunction : arg


  //---------------------------------------------------------------------------
  // Fixed-Point Complex Arithmetic
  //---------------------------------------------------------------------------

  // Add two complex numbers: x + y
  function sc16_t add_sc16(sc16_t x, sc16_t y);
    return complex_to_sc16(add(sc16_to_complex(x), sc16_to_complex(y)));
  endfunction : add_sc16

  // Subtract two complex numbers: x - y
  function sc16_t sub_sc16(sc16_t x, sc16_t y);
    return complex_to_sc16(sub(sc16_to_complex(x), sc16_to_complex(y)));
  endfunction : sub_sc16

  // Multiply two complex numbers: x * y
  function sc16_t mul_sc16(sc16_t x, sc16_t y);
    return complex_to_sc16(mul(sc16_to_complex(x), sc16_to_complex(y)));
  endfunction : mul_sc16

  // Divide two complex numbers: x / y
  function sc16_t div_sc16(sc16_t x, sc16_t y);
    return complex_to_sc16(div(sc16_to_complex(x), sc16_to_complex(y)));
  endfunction : div_sc16

  // Compute the exponential: e^x
  function sc16_t exp_sc16(sc16_t x);
    return complex_to_sc16(exp(sc16_to_complex(x)));
  endfunction : exp_sc16

  // Compute the sine: sin(x)
  function sc16_t sin_sc16(sc16_t x);
    return complex_to_sc16(sin(sc16_to_complex(x)));
  endfunction : sin_sc16

  // compute the cosine: cos(x)
  function sc16_t cos_sc16(sc16_t x);
    return complex_to_sc16(cos(sc16_to_complex(x)));
  endfunction : cos_sc16

  // Compute the magnitude/modulus/absolute value: |x|
  function s16_t mag_sc16(sc16_t x);
    return real_to_s16(mag(sc16_to_complex(x)));
  endfunction : mag_sc16

  // Compute the phase/argument: arg(x)
  function s16_t arg_sc16(sc16_t x);
    return real_to_s16(arg(sc16_to_complex(x)));
  endfunction : arg_sc16

endpackage : PkgComplex

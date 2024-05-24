//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_reorder_pkg
//
// Description:
//
//   Package file for the fft_reorder module. Includes relevant types and
//   functions needed by the module.
//

package fft_reorder_pkg;

  typedef enum bit [1:0] {
    NORMAL,
    REVERSE,
    NATURAL,
    BIT_REVERSE
  } fft_order_t;

  // Reverse the order of the lower `width` bits on the `index` input. The
  // upper bits will be 0.
  function automatic int bit_reverse (bit [15:0] index, bit [3:0] width);
    bit [15:0] result;

    // Reverse bit order
    result = { << { index }};

    // Right-align
    result = result >> (16 - width);

    return result;
  endfunction

endpackage : fft_reorder_pkg

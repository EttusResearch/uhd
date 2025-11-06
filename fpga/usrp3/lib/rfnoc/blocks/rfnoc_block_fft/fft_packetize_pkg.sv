//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_packetize_pkg
//
// Description:
//
//   Package file for fft_packetize and fft_depacketize.
//


package fft_packetize_pkg;

  import rfnoc_chdr_utils_pkg::*;

  typedef struct packed {
    logic [CHDR_TIMESTAMP_W-1:0] timestamp;
    logic                        has_time;
    logic [CHDR_LENGTH_W-1:0]    length;
  } burst_info_t;

  typedef struct packed {
    logic last;
  } symbol_info_t;

  localparam int BURST_INFO_W  = $bits(burst_info_t);
  localparam int SYMBOL_INFO_W = $bits(symbol_info_t);

endpackage : fft_packetize_pkg

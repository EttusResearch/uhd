//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: ioport2_msg_pkg
//
// Description:
//
//   Shared packed-struct definition for ioport2 message encoding.
//

package ioport2_msg_pkg;

  typedef struct packed {
    logic        rd_response;
    logic        wr_request;
    logic        rd_request;
    logic        half_word;
    logic [ 7:0] reserved;
    logic [19:0] address;
    logic [31:0] data;
  } ioport2_msg_t;

endpackage : ioport2_msg_pkg

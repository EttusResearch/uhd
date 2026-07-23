//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_ioport2_stream_all_tb
//
// Description:
//
//   Top-level testbench for ctrlport_to_ioport2_stream. Runs two parameter
//   variants.
//

`default_nettype none

module ctrlport_to_ioport2_stream_all_tb;

  ctrlport_to_ioport2_stream_tb #(
    .SIGNATURE(32'h1357_2468),
    .CLK_FREQ (32'd125_000_000)
  ) tb_0 ();

  ctrlport_to_ioport2_stream_tb #(
    .SIGNATURE(32'h89AB_CDEF),
    .CLK_FREQ (32'd200_000_000)
  ) tb_1 ();

endmodule : ctrlport_to_ioport2_stream_all_tb

`default_nettype wire

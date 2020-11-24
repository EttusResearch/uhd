//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: glitch_free_mux
//
// Description:
//   This module implements a 2:1 mux in a LUT explicitly to avoid glitches
//   which can be introduced by unexpected Vivado synthesis.
//

module glitch_free_mux (
  input  wire select,
  input  wire signal0,
  input  wire signal1,
  output wire muxed_signal
);

  (* dont_touch = "TRUE" *) LUT3 #(
    .INIT(8'hCA) // Specify LUT Contents. O = ~I2&I0 | I2&I1
  ) mux_out_i (
    .O (muxed_signal), // LUT general output. Mux output
    .I0(signal0),      // LUT input. Input 1
    .I1(signal1),      // LUT input. Input 2
    .I2(select)        // LUT input. Select bit
  );

endmodule

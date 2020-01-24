//
// Copyright 2011 Ettus Research LLC
// Copyright 2018-2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// The purpose of this module is to synchronize a reset from one clock domain
// to another. The reset_in signal must be driven by a glitch-free source.
//

module reset_sync (
  // clock for the output reset
  input  clk,
  // glitch-free input reset
  input  reset_in,
  // output reset in the clk domain
  output reg reset_out);

  wire reset_c;

  synchronizer #(
    // The input reset is async to the output clk domain... so timing should not be
    // analyzed here!
    .FALSE_PATH_TO_IN(1),
    // Assert reset_out by default. When clk starts toggling the downstream logic will
    // be in reset for at least 10 clk cycles. This allows the clock to settle (if needed)
    // and the reset to propagate fully to all logic.
    .INITIAL_VAL(1),
    .STAGES(10)
  ) reset_double_sync (
    .clk(clk), .rst(1'b0), .in(reset_in), .out(reset_c)
  );

  always @(posedge clk)
      reset_out <= reset_c;

endmodule // reset_sync

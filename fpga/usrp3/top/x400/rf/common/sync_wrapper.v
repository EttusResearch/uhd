//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: sync_wrapper
//
// Description:
//
//   As the original synchronizer component has port signal names that are
//   incompatible with VHDL (in, out), this modules provides an an interface to
//   instantiate the synchronizer block in VHDL.
//

`default_nettype none

module sync_wrapper #(
   parameter WIDTH            = 1,
   parameter STAGES           = 2,
   parameter INITIAL_VAL      = 0,
   parameter FALSE_PATH_TO_IN = 1
)(
   input  wire             clk,
   input  wire             rst,
   input  wire [WIDTH-1:0] signal_in,
   output wire [WIDTH-1:0] signal_out
);

synchronizer #(
  .WIDTH             (WIDTH),
  .STAGES            (STAGES),
  .INITIAL_VAL       (INITIAL_VAL),
  .FALSE_PATH_TO_IN  (FALSE_PATH_TO_IN)
) synchronizer_i (
  .clk  (clk),
  .rst  (rst),
  .in   (signal_in),
  .out  (signal_out)
);

endmodule //sync_wrapper

`default_nettype wire

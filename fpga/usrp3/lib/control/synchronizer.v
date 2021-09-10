//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: synchronizer.v
//
// Description:
//
//   This is a double-synchronizer module. However, it can can implement a
//   synchronizer of any depth (double, triple, etc.) and width.
//
//   A double synchronizer is typically used to cross a single-bit glitch-free
//   signal from one clock domain to another.
//
//   *WARNING*: The input signal must be glitch-free. In other words, it should
//              be driven by a register and NOT combinational logic. Otherwise
//              you could capture a glitch instead of the intended signal.
//
//   *WARNING*: When WIDTH is not 1, the multiple bits are not guaranteed to be
//              coherent. In other words, they can arrive on the output at
//              different times. This module should not usually be used to
//              cross a multi-bit signal. Consider using the handshake module
//              instead.
//
//   When crossing between unrelated clock domains, we typically don't want the
//   timing analyzer to consider the path between clock domains. To make
//   writing this constraint easier, the FALSE_PATH_TO_IN parameter controls
//   the name of the synchronizer_impl instance. The following XDC constraint
//   is used to ignore all instances of this false path.
//
//     set_false_path -to [get_pins -hierarchical -filter \
//       {NAME =~ */synchronizer_false_path/stages[0].value_reg[0][*]/D}]
//
// Parameters:
//
//   WIDTH            : Width of the synchronizer (1 by default).
//   STAGES           : Number of synchronizer stages (2 by default, for a
//                      standard double-synchronizer).
//   INITIAL_VAL      : Initial value of the output register (0 by default).
//   FALSE_PATH_TO_IN : Set to 1 if the input should be considered a false path
//                      and ignored by the timing analyzer. Set to 0 to let the
//                      tool analyze this path.
//

module synchronizer #(
   parameter WIDTH            = 1,
   parameter STAGES           = 2,
   parameter INITIAL_VAL      = 0,
   parameter FALSE_PATH_TO_IN = 1
)(
   input              clk,
   input              rst,
   input  [WIDTH-1:0] in,
   output [WIDTH-1:0] out
);

   generate if (FALSE_PATH_TO_IN == 1) begin
      synchronizer_impl #(
        .WIDTH(WIDTH), .STAGES(STAGES), .INITIAL_VAL(INITIAL_VAL)
      ) synchronizer_false_path (
         .clk(clk), .rst(rst), .in(in), .out(out)
      );
   end else begin
      synchronizer_impl #(
        .WIDTH(WIDTH), .STAGES(STAGES), .INITIAL_VAL(INITIAL_VAL)
      ) synchronizer_constrained (
         .clk(clk), .rst(rst), .in(in), .out(out)
      );
   end endgenerate

endmodule   //synchronizer

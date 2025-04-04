//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: synchronizer_sync_in
//
// Description:
//
//   This is a synchronizer module designed for a synchronous input. This
//   synchronizer should be used any time you want to cross a synchronous
//   signal that is not driven by a flip-flop to another clock domain.
//
//   Combinational logic outputs require a flip-flop to filter any glitches
//   before being crossed to another clock domain. This module consists of a
//   standard "synchronizer" module with an input flip-flop on the driving
//   clock domain to filter these glitches. A block diagram of this module is
//   shown below.
//
//            Input Flip-Flop            Synchronizer
//
//       i_sig     ┌───┐                ┌───┐    ┌───┐ o_sig
//       ─────────►│   ├───────────────►│   ├───►│   ├─────►
//              ┌─►│   │           ┌───►│   │ ┌─►│   │
//              │  └───┘           │    └───┘ │  └───┘
//              │                  │          │
//       i_clk  │                  │          │
//       ───────┘                  │          │
//                                 │          │
//       o_clk                     │          │
//       ──────────────────────────┴──────────┘
//
// Parameters:
//
//   WIDTH       : Width of the synchronizer (1 by default). Note that this
//                 module does not ensure coherence between bits. Multi-bit
//                 signals should normally use the handshake module instead of
//                 this module to ensure that all output bits arrive at the
//                 same time.
//   STAGES      : Number of synchronizer stages (3 by default, for a
//                 triple-synchronizer).
//   INITIAL_VAL : Initial and reset value of the registers (0 by default).
//

`default_nettype none


module synchronizer_sync_in #(
   int   WIDTH            = 1,
   int   STAGES           = 3,
   logic INITIAL_VAL      = 0
) (
   input  wire             i_clk,
   input  wire             i_rst,
   input  wire [WIDTH-1:0] i_sig,

   input  wire             o_clk,
   input  wire             o_rst,
   output wire [WIDTH-1:0] o_sig
);
   logic [WIDTH-1:0] i_sig_reg = { WIDTH {INITIAL_VAL}};

   // Filter any glitches from combinational logic by registering the input
   always_ff @(posedge i_clk) begin
     if (i_rst) begin
       i_sig_reg <= { WIDTH {INITIAL_VAL}};
     end else begin
       i_sig_reg <= i_sig;
     end
   end

   synchronizer #(
      .WIDTH           (WIDTH      ),
      .STAGES          (STAGES     ),
      .INITIAL_VAL     (INITIAL_VAL),
      .FALSE_PATH_TO_IN(1          )
   ) synchronizer_i (
      .clk(o_clk    ),
      .rst(o_rst    ),
      .in (i_sig_reg),
      .out(o_sig    )
   );

endmodule : synchronizer_sync_in


`default_nettype wire

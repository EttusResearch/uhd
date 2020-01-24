//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: pulse_generator
// Description:
// Generates pulses of a given width at intervals of a given period based on
// a given input clock.

module pulse_generator #(parameter WIDTH = 32) (
  input  wire             clk,          /* clock */
  input  wire             reset,        /* reset */
  input  wire [WIDTH-1:0] period,       /* period, in clk cycles */
  input  wire [WIDTH-1:0] pulse_width,  /* pulse width, in clk cycles */
  output reg              pulse         /* pulse */
);
  reg [WIDTH-1:0] count = 0;

  always @(posedge clk) begin
    if (reset | count <= 1)
      count <= period;
    else
      count <= count - 1;

    pulse <= (count > (period - pulse_width));
  end

endmodule //pulse_generator

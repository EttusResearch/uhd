//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: clock_div
//
// Description:
//  Clock divider. Generates an output clock that is 1/N times the frequency
//  of the input clock.
//
// Parameters:
//
//   N : Even number to divide clock by
//

module clock_div #(
  parameter N = 8'h1
) (
  // Input Clock and reset
  input  wire        clk_in,
  input  wire        clk_in_rst,

  // Request
  output reg        clk_out
);
  localparam NDIV2 = N/2;
  localparam WIDTH = $clog2(NDIV2);
  reg [WIDTH-1:0] div_ctr;
  wire [WIDTH-1:0] next_div_ctr;

  //errors if N is not even
  if (N % 2 != 0) begin : gen_assertion
    ERROR_N_must_be_even();
  end

  always @(posedge clk_in) begin

    if(clk_in_rst) begin
      div_ctr <= 0;
      clk_out <= 0;
    end else if (next_div_ctr == NDIV2) begin
      div_ctr <= 0;
      clk_out <= ~clk_out;
    end else begin
      div_ctr <= next_div_ctr;
    end

  end

  assign next_div_ctr = div_ctr + 1;

endmodule

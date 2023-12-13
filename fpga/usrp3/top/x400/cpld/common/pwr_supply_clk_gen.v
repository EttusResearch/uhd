//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: pwr_supply_clk_gen
//
// Description:
//
//   Generates a clock for one motherboard power supply.
//
// Parameters:
//
//   BASE_ADDRESS  : Base address for CtrlPort registers.
//   NUM_ADDRESSES : Number of bytes of address space to use.
//

`default_nettype none


module pwr_supply_clk_gen#(
  parameter SOURCE_CLK_FREQ = 100_000_000,
  parameter TARGET_CLK_FREQ =     100_000
) (
  // Base clock and reset
  input wire clk,
  input wire rst,

  // Power supply clocks
  output reg pwr_supply_clk
);

//-----------------------------------------------------------------------------
// Counter Calculation / Definition
//-----------------------------------------------------------------------------
// Counter to generate the power supply switching clock

// Assumption: the ratio between the generated clock and the source clock is
//             even, therefore we can produce a 50% DC clock output.
localparam MAX_COUNT = SOURCE_CLK_FREQ / TARGET_CLK_FREQ / 2;
localparam COUNTER_W = $clog2(MAX_COUNT);
reg [COUNTER_W-1:0] counter = 0;

//-----------------------------------------------------------------------------
// Clock Generation
//-----------------------------------------------------------------------------
// This process implements a simple clock divider for the power supply
// switcher.

// SAFE COUNTER START! rst is a synchronous reset generated in the
// clk domain; therefore, inherently safe.
always @(posedge clk) begin
  if (rst) begin
    counter <= 0;
    pwr_supply_clk <= 1'b0;
  end
  else begin
    // Add one every cycle to the counter
    counter <= counter + 1'b1;

    // When the counter reaches its mid value, it is reset and the output clock
    // output is toggled.
    if (counter == MAX_COUNT-1) begin
      counter <= 0;
      pwr_supply_clk <= ~pwr_supply_clk;
    end
  end
end

endmodule


`default_nettype wire

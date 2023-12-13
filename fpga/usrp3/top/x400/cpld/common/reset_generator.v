//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: reset_generator
//
// Description:
//
//   Generates a power-on reset signal that is asserted at startup and stays
//   asserted for at least CYCLES_IN_RESET clock cycles.
//
//   Internally, it generates a 1-bit synchronous signal (initialize) to safely
//   initialize the power_on_reset_counter incremental counter to 0's.
//
//   A delayed version of the initializing signal is also generated
//   (counter_enable) to start counting.
//
//                   1_  2_  3_  4_  5_  6_  7_  8_  9_
//             clk  _| |_| |_| |_| |_| |_| |_| |_| |_|
//                                _____________________
//       initialize _____________|
//                                            _________
//   counter_enable _________________________|
//

`default_nettype none


module reset_generator (
  input  wire clk,
  output reg  power_on_reset = 1'b1
);
  
  wire [0:0] counter_enable;
  wire [0:0] initialize;

  synchronizer #(
    .WIDTH            (1),
    .STAGES           (3),
    .INITIAL_VAL      (1'b0),
    .FALSE_PATH_TO_IN (0)
  ) init_sync_inst (
    .clk (clk),
    .rst (1'b0),
    .in  (1'b1),
    .out (initialize)
  );

  synchronizer #(
    .WIDTH            (1),
    .STAGES           (3),
    .INITIAL_VAL      (1'b0),
    .FALSE_PATH_TO_IN (0)
  ) counter_en_sync_inst (
    .clk (clk),
    .rst (1'b0),
    .in  (initialize),
    .out (counter_enable)
  );

  // Internal synchronous reset generator.
  localparam CYCLES_IN_RESET = 20;
  reg [7:0] power_on_reset_counter = 8'b0;

  // This block generates a synchronous reset in the clk domain that can be
  // used by downstream logic.
  //
  // power_on_reset_counter is first initialized to 0's upon assertion of
  // initialize. Some cycles later (3), upon assertion if counter_enable,
  // power_on_reset_counter starts to increment.
  //
  // power_on_reset will remain asserted until power_on_reset_counter reaches
  // cycles_in_reset, resulting in the deassertion of power_on_reset.
  always @(posedge clk) begin : power_on_reset_gen
    if (counter_enable) begin
      if (power_on_reset_counter == CYCLES_IN_RESET-1) begin
        power_on_reset <= 1'b0;
      end else begin
        power_on_reset_counter <= power_on_reset_counter + 1'b1;
        power_on_reset <= 1'b1;
      end
    end
    else if (initialize) begin
      power_on_reset_counter <= 8'b0;
      power_on_reset <= 1'b1;
    end
  end

endmodule


`default_nettype wire

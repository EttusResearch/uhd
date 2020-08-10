///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: handshake
// Description:
// Implements clock domain crossing for data from one clock domain
// to another independent of the relative clock frequencies or phases of
// clk_a and clk_b.
//
// Once a handshake is triggered it cannot be aborted. A proper design needs to
// apply reset to the target clock domain downstream module to make sure a
// valid_b pulse does not cause any issues.
//
// Input Behavior:
//          ┌┐┌┐┌┐┌┐┋ ┋┌┐┌┐┌┐┌┐┌┐┋ ┋┌┐┌┐┌┐┌┐┌┐┌┐┌┐
//  clk_a   ┘└┘└┘└┘└┋ ┋┘└┘└┘└┘└┘└┋ ┋┘└┘└┘└┘└┘└┘└┘└
//            ┌─┐   ┋ ┋    ┌─┐   ┋ ┋
//  valid_a ──┘ └───┋ ┋────┘ └───┋ ┋──────────────
//          ▄▄┬─┬▄▄▄┋ ┋▄▄▄▄┬─┬▄▄▄┋ ┋▄▄▄▄▄▄▄▄▄▄▄▄▄▄
//  data_a  ▀▀┴─┴▀▀▀┋ ┋▀▀▀▀┴─┴▀▀▀┋ ┋▀▀▀▀▀▀▀▀▀▀▀▀▀▀
//              ┌───┋ ┋────┐ ┌───┋ ┋────────┐
//  busy_a  ────┘   ┋ ┋    └─┘   ┋ ┋        └─────
//
//
// Output Behavior:
//           ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┋ ┋ ┌─┐ ┌─┐ ┌─┐ ┌─┐
//  clk_b   ─┘ └─┘ └─┘ └─┘ └─┋ ┋─┘ └─┘ └─┘ └─┘ └─
//                   ┌───┐   ┋ ┋     ┌───┐
//  valid_b ─────────┘   └───┋ ┋─────┘   └───────
//          ▄▄▄▄▄▄▄▄▄┬───┬▄▄▄┋ ┋▄▄▄▄▄┬───┬▄▄▄▄▄▄▄
//  data_b  ▀▀▀▀▀▀▀▀▀┴───┴▀▀▀┋ ┋▀▀▀▀▀┴───┴▀▀▀▀▀▀▀
///////////////////////////////////////////////////////////////////////////////

module handshake #(
  parameter WIDTH = 32 // data width
) (
  // source clock domain
  input  wire             clk_a,
  input  wire             rst_a,
  input  wire             valid_a, // trigger handshake on rising edge
  input  wire [WIDTH-1:0] data_a,
  output wire             busy_a,

  // target clock domain
  input  wire             clk_b,
  output wire             valid_b,
  output wire [WIDTH-1:0] data_b
);

  // Handling of the handshaking between the two clock domains. The reset does
  // not delete a pulse, which is already triggered!
  pulse_synchronizer #(
    .MODE("PULSE"), .STAGES(4)
  ) push_sync_inst (
     .clk_a(clk_a), .rst_a(rst_a), .pulse_a(valid_a), .busy_a(busy_a),
     .clk_b(clk_b), .pulse_b(valid_b)
  );

  // Capture the data aligned with triggering the handshake.
  reg [WIDTH-1:0] data_a_lcl;
  always @(posedge clk_a) begin
    if (valid_a & ~busy_a) begin
      data_a_lcl <= data_a;
    end
  end

  // Transfer data with timing exception. Data is captured upfront and kept
  // stable in clk_a domain. As there are more synchronizer stages in the
  // pulse_synchronizer the data in these 2 stage synchronizer is stable once
  // the valid_b pulse arrives in clk_b domain. 2 stages are used to resolve
  // meta-stability. Reset is not needed on the data path as it is controlled by
  // the valid signals.
  synchronizer #(
    .WIDTH(WIDTH), .STAGES(2), .INITIAL_VAL({WIDTH {1'b0}}), .FALSE_PATH_TO_IN(1)
  ) data_sync_inst (
    .clk(clk_b), .rst(1'b0), .in(data_a_lcl), .out(data_b)
  );

endmodule

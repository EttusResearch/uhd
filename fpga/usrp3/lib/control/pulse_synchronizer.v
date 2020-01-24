//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: pulse_synchronizer
// Description:
// - Synchronizes a single-cycle pulse or an edge from one
//   clock domain to another
// - Clocks A and B can be asynchronous
//

module pulse_synchronizer #(
  parameter MODE   = "PULSE", // Capture mode {PULSE, POSEDGE, NEGEDGE}
  parameter STAGES = 2        // Number of synchronizer stages
) (
  input   clk_a,    // Clock A
  input   rst_a,    // Reset in clock domain A
  input   pulse_a,  // Pulse in clock domain A to synchronize
  output  busy_a,   // Synchronizer is busy (pulse_a ignored when asserted)
  input   clk_b,    // Clock B
  output  pulse_b   // Pulse in clock domain B
);
  // Trigger logic based on the capture mode
  wire trigger;
  generate if (MODE == "POSEDGE") begin
    reg pulse_a_del_pe = 1'b0;
    always @ (posedge clk_a)
      pulse_a_del_pe <= rst_a ? 1'b0 : pulse_a;
    assign trigger = pulse_a & ~pulse_a_del_pe;
  end else if (MODE == "NEGEDGE") begin
    reg pulse_a_del_ne = 1'b1;
    always @ (posedge clk_a)
      pulse_a_del_ne <= rst_a ? 1'b1 : pulse_a;
    assign trigger = ~pulse_a & pulse_a_del_ne;
  end else begin
    assign trigger = pulse_a;
  end endgenerate

  // Translate pulse/edge to a level and synchronize that into the B domain
  reg pulse_toggle_a = 1'b0;
  always @(posedge clk_a) begin
    pulse_toggle_a <= rst_a ? 1'b0 : (pulse_toggle_a ^ (trigger & ~busy_a));
  end

  wire pulse_toggle_b;
  reg  pulse_toggle_b_del = 1'b0;
  wire handshake_toggle_a;

  synchronizer #(
    .STAGES(STAGES), .INITIAL_VAL(0)
  ) toggle_sync_i (
    .clk(clk_b), .rst(1'b0), .in(pulse_toggle_a), .out(pulse_toggle_b)
  );

  // Handshake toggle signal back into the A domain to deassert busy
  synchronizer #(
    .STAGES(STAGES), .INITIAL_VAL(0)
  ) handshake_sync_i (
    .clk(clk_a), .rst(1'b0), .in(pulse_toggle_b_del), .out(handshake_toggle_a)
  );

  always @(posedge clk_b) begin
    pulse_toggle_b_del <= pulse_toggle_b;
  end

  assign pulse_b = pulse_toggle_b_del ^ pulse_toggle_b;
  assign busy_a = pulse_toggle_a ^ handshake_toggle_a;

endmodule
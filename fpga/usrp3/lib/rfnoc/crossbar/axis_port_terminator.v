//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_port_terminator
// Description:
//   A dummy terminator for unused crossbar ports

module axis_port_terminator #(
  parameter DATA_W = 64
) (
  // Clocks and resets
  input  wire               clk,
  input  wire               reset,
  // Input ports
  input  wire [DATA_W-1:0]  s_axis_tdata,   // Input data
  input  wire               s_axis_tlast,   // Input EOP (last)
  input  wire               s_axis_tvalid,  // Input valid
  output wire               s_axis_tready,  // Input ready
  // Output ports
  output wire [DATA_W-1:0]  m_axis_tdata,   // Output data
  output wire               m_axis_tlast,   // Output EOP (last) 
  output wire               m_axis_tvalid,  // Output valid
  input  wire               m_axis_tready,  // Output ready
  // Metrics
  output reg  [15:0]        pkts_dropped
);

  assign s_axis_tready  = 1'b1;
  assign m_axis_tdata   = {DATA_W{1'b0}};
  assign m_axis_tlast   = 1'b0;
  assign m_axis_tvalid  = 1'b0;

  always @(posedge clk) begin
    if (reset) begin
      pkts_dropped <= 'd0;
    end else if (s_axis_tvalid & s_axis_tlast & s_axis_tready) begin
      pkts_dropped <= pkts_dropped + 'd1;
    end
  end

endmodule


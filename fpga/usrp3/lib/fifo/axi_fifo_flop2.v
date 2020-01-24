//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Single cycle latency, depth of 2 "Flip flop" with no end to end combinatorial paths on
// AXI control signals (such as i_tready depends on o_tready). Breaking the combinatorial
// paths requires an additional register stage.
//
// Note: Once i_tvalid is asserted, it cannot be deasserted without i_tready having asserted
//       indicating i_tdata has been read. This is an AXI stream requirement.

module axi_fifo_flop2 #(
  parameter WIDTH = 32
)(
  input clk,
  input reset,
  input clear,
  input [WIDTH-1:0] i_tdata,
  input i_tvalid,
  output i_tready,
  output reg [WIDTH-1:0] o_tdata = 'h0,
  output reg o_tvalid = 1'b0,
  input o_tready,
  output [1:0] space,
  output [1:0] occupied);

  reg [WIDTH-1:0] i_tdata_temp = 'h0;
  reg             i_tvalid_temp = 1'b0;

  assign i_tready = ~i_tvalid_temp;

  always @(posedge clk) begin
    if (~o_tvalid | o_tready) begin
      if (i_tvalid_temp) begin
        o_tvalid      <= 1'b1;
        o_tdata       <= i_tdata_temp;
      end else begin
        o_tvalid      <= i_tvalid;
        o_tdata       <= i_tdata;
      end
      i_tvalid_temp   <= 1'b0;
    end else begin
      if (~i_tvalid_temp) begin
        i_tvalid_temp <= i_tvalid;
        i_tdata_temp  <= i_tdata;
      end
    end
    if (reset | clear) begin
      o_tvalid      <= 1'b0;
      i_tvalid_temp <= 1'b0;
    end
  end

  assign occupied = i_tvalid_temp + o_tvalid;
  assign space    = 2 - occupied;

endmodule

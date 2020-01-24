//
// Copyright 2016 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module axi_round_complex #(
  parameter WIDTH_IN  = 24,
  parameter WIDTH_OUT = 16,
  parameter FIFOSIZE  = 0)  // leave at 0 for a normal single flop
(
  input clk, input reset,
  input [2*WIDTH_IN-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [2*WIDTH_OUT-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  wire [WIDTH_IN-1:0]  ii_tdata, iq_tdata;
  wire                 ii_tlast, ii_tvalid, ii_tready, iq_tlast, iq_tvalid, iq_tready;

  wire [WIDTH_OUT-1:0] oi_tdata, oq_tdata;
  wire                 oi_tlast, oi_tvalid, oi_tready, oq_tlast, oq_tvalid, oq_tready;

  split_complex #(.WIDTH(WIDTH_IN)) split_complex (
    .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
    .oi_tdata(ii_tdata), .oi_tlast(ii_tlast), .oi_tvalid(ii_tvalid), .oi_tready(ii_tready),
    .oq_tdata(iq_tdata), .oq_tlast(iq_tlast), .oq_tvalid(iq_tvalid), .oq_tready(iq_tready));

  axi_round #(.WIDTH_IN(WIDTH_IN), .WIDTH_OUT(WIDTH_OUT), .FIFOSIZE(FIFOSIZE)) axi_round_i (
    .clk(clk), .reset(reset),
    .i_tdata(ii_tdata), .i_tlast(ii_tlast), .i_tvalid(ii_tvalid), .i_tready(ii_tready),
    .o_tdata(oi_tdata), .o_tlast(oi_tlast), .o_tvalid(oi_tvalid), .o_tready(oi_tready));

  axi_round #(.WIDTH_IN(WIDTH_IN), .WIDTH_OUT(WIDTH_OUT), .FIFOSIZE(FIFOSIZE)) axi_round_q (
    .clk(clk), .reset(reset),
    .i_tdata(iq_tdata), .i_tlast(iq_tlast), .i_tvalid(iq_tvalid), .i_tready(iq_tready),
    .o_tdata(oq_tdata), .o_tlast(oq_tlast), .o_tvalid(oq_tvalid), .o_tready(oq_tready));

  join_complex #(.WIDTH(WIDTH_OUT)) join_complex (
    .ii_tdata(oi_tdata), .ii_tlast(oi_tlast), .ii_tvalid(oi_tvalid), .ii_tready(oi_tready),
    .iq_tdata(oq_tdata), .iq_tlast(oq_tlast), .iq_tvalid(oq_tvalid), .iq_tready(oq_tready),
    .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));

endmodule

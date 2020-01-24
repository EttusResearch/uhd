//
// Copyright 2014, Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//
// Reduce the bitwidth of an input stream. This module will both round
// and clip, meaning that the lower bits will be removed by rounding to
// the nearest value, and the top bits will be snipped (but without
// causing numerical overflows).
//
// Example: If WIDTH_IN==24, WIDTH_OUT==16, and CLIP_BITS==3, the output
// Will remove the top 3 bits (by clipping), and remove the bottom 5
// bits by rounding, leaving 24 - 3 - 5 == 16 bits.
//
// Note that this module has two stages (round, then clip) which will
// both have a FIFO of length FIFOSIZE. However, when a stage is not
// required it will have *no* FIFO, but instead just pass through the
// data. In the extreme case where WIDTH_IN==WIDTH_OUT and CLIP_BITS==0,
// there are no FIFOs and this module becomes an AXI stream passthrough.
module axi_round_and_clip
#(
  parameter WIDTH_IN=24,
  parameter WIDTH_OUT=16,
  parameter CLIP_BITS=3,
  parameter FIFOSIZE=1)  // FIFOSIZE = 1, single output register
(
  input clk, input reset,
  input [WIDTH_IN-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [WIDTH_OUT-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  wire [WIDTH_OUT+CLIP_BITS-1:0] int_tdata;
  wire int_tlast, int_tvalid, int_tready;

  generate
    if (WIDTH_IN == WIDTH_OUT+CLIP_BITS) begin
       assign int_tdata    = i_tdata;
       assign int_tlast    = i_tlast;
       assign int_tvalid   = i_tvalid;
       assign i_tready     = int_tready;
    end else begin
      axi_round #(
        .WIDTH_IN(WIDTH_IN), .WIDTH_OUT(WIDTH_OUT+CLIP_BITS),
        .round_to_nearest(1), .FIFOSIZE(FIFOSIZE))
      axi_round (
        .clk(clk), .reset(reset),
        .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
        .o_tdata(int_tdata), .o_tlast(int_tlast), .o_tvalid(int_tvalid), .o_tready(int_tready));
    end

    if (CLIP_BITS == 0) begin
      assign o_tdata    = int_tdata;
      assign o_tlast    = int_tlast;
      assign o_tvalid   = int_tvalid;
      assign int_tready = o_tready;
    end else begin
      axi_clip #(
        .WIDTH_IN(WIDTH_OUT+CLIP_BITS), .WIDTH_OUT(WIDTH_OUT),
        .FIFOSIZE(FIFOSIZE))
      axi_clip (
        .clk(clk), .reset(reset),
        .i_tdata(int_tdata), .i_tlast(int_tlast), .i_tvalid(int_tvalid), .i_tready(int_tready),
        .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));
    end
  endgenerate

endmodule // round_and_clip

//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module moving_sum #(
  parameter MAX_LEN = 1023,
  parameter WIDTH   = 16
)(
  input clk, input reset, input clear,
  input [$clog2(MAX_LEN+1)-1:0] len,
  input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [WIDTH+$clog2(MAX_LEN+1)-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  wire signed [WIDTH+$clog2(MAX_LEN+1)-1:0] sum;
  reg signed [WIDTH+$clog2(MAX_LEN+1)-1:0] sum_reg;
  reg [$clog2(MAX_LEN+1)-1:0] full_count, len_reg;
  reg len_changed;

  wire full  = (full_count == len_reg);
  wire do_op = (i_tvalid & i_tready);

  wire i_tready_int, i_tvalid_int;
  wire fifo_tvalid, fifo_tready;
  wire [WIDTH-1:0] fifo_tdata;

  axi_fifo #(.WIDTH(WIDTH), .SIZE($clog2(MAX_LEN))) axi_fifo (
    .clk(clk), .reset(reset | len_changed), .clear(clear),
    .i_tdata(i_tdata), .i_tvalid(do_op), .i_tready(),
    .o_tdata(fifo_tdata), .o_tvalid(fifo_tvalid), .o_tready(fifo_tready),
    .occupied(), .space());

  assign fifo_tready = i_tvalid & i_tready_int & full;

  always @(posedge clk) begin
    if (reset | clear | len_changed) begin
       full_count <= 'd0;
    end else begin
      if (do_op & ~full) begin
        full_count <= full_count + 1;
      end
    end
  end

  assign sum = sum_reg + $signed(i_tdata) - (full ? $signed(fifo_tdata) : 0);

  always @(posedge clk) begin
    if (reset | clear) begin
      sum_reg     <= 'd0;
      len_reg     <= 1;
      len_changed <= 1'b0;
    end else begin
      len_reg <= (len == 0) ? 1 : len;
      if (len_reg != len) begin
        len_changed <= 1'b1;
      end else begin
        len_changed <= 1'b0;
      end
      if (len_changed) begin
        sum_reg <= 'd0;
      end else if (do_op) begin
        sum_reg <= sum;
      end
    end
  end

  // Output register
  axi_fifo_flop #(.WIDTH(WIDTH+$clog2(MAX_LEN+1)+1)) axi_fifo_flop (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({i_tlast,sum}), .i_tvalid(i_tvalid_int), .i_tready(i_tready_int),
    .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
    .occupied(), .space());

  assign i_tready     = (~full | (fifo_tvalid & full)) & i_tready_int & ~len_changed;
  assign i_tvalid_int = (~full | (fifo_tvalid & full)) & i_tvalid     & ~len_changed;

endmodule // moving_sum

//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Demonstration of two input, two output block

module addsub
  #(parameter WIDTH = 16)
   (input clk, input reset,
    input [WIDTH*2-1:0] i0_tdata, input i0_tlast, input i0_tvalid, output i0_tready,
    input [WIDTH*2-1:0] i1_tdata, input i1_tlast, input i1_tvalid, output i1_tready,
    output [WIDTH*2-1:0] sum_tdata, output sum_tlast, output sum_tvalid, input sum_tready,
    output [WIDTH*2-1:0] diff_tdata, output diff_tlast, output diff_tvalid, input diff_tready);

   wire [WIDTH*4-1:0] 	 dummy;
   wire [WIDTH*4-1:0] 	 int_tdata;
   wire 		 int_tlast, int_tvalid, int_tready;

   assign int_tvalid = i0_tvalid & i1_tvalid;
   assign i0_tready = int_tvalid & int_tready;
   assign i1_tready = int_tvalid & int_tready;

   wire [WIDTH-1:0] 	 sum_a = i0_tdata[WIDTH*2-1:WIDTH] + i1_tdata[WIDTH*2-1:WIDTH];
   wire [WIDTH-1:0] 	 diff_a = i0_tdata[WIDTH*2-1:WIDTH] - i1_tdata[WIDTH*2-1:WIDTH];
   
   wire [WIDTH-1:0] 	 sum_b = i0_tdata[WIDTH-1:0] + i1_tdata[WIDTH-1:0];
   wire [WIDTH-1:0] 	 diff_b = i0_tdata[WIDTH-1:0] - i1_tdata[WIDTH-1:0];

   assign int_tdata = { sum_a,sum_b,diff_a,diff_b };
   assign int_tlast = i0_tlast;  // Follow first input.
   
   split_stream_fifo #(.WIDTH(4*WIDTH), .ACTIVE_MASK(4'b0011)) splitter
     (.clk(clk), .reset(reset), .clear(1'b0),
      .i_tdata(int_tdata), .i_tlast(int_tlast), .i_tvalid(int_tvalid), .i_tready(int_tready),
      .o0_tdata({sum_tdata,dummy[WIDTH*2-1:0]}), .o0_tlast(sum_tlast), .o0_tvalid(sum_tvalid), .o0_tready(sum_tready),
      .o1_tdata({dummy[WIDTH*4-1:WIDTH*2],diff_tdata}), .o1_tlast(diff_tlast), .o1_tvalid(diff_tvalid), .o1_tready(diff_tready),
      .o2_tready(1'b1), .o3_tready(1'b1));
   
endmodule // addsub

//
// Copyright 2012-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//



// Cascade FIFO :  ShortFIFO -> Block RAM fifo -> ShortFIFO for timing and placement help

// Special case SIZE <= 5 uses a short fifo in the middle, which is not too useful in this case

module axi_fifo_cascade
  #(parameter WIDTH=32, SIZE=9)
   (input clk, input reset, input clear,
    input [WIDTH-1:0] i_tdata,
    input i_tvalid,
    output i_tready,
    output [WIDTH-1:0] o_tdata,
    output o_tvalid,
    input o_tready,
    
    output [15:0] space,
    output [15:0] occupied);

   wire [WIDTH-1:0]   int1_tdata, int2_tdata;
   wire 	      int1_tvalid, int1_tready, int2_tvalid, int2_tready;
   
   axi_fifo_flop2 #(.WIDTH(WIDTH)) pre_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata(i_tdata), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o_tdata(int1_tdata), .o_tvalid(int1_tvalid), .o_tready(int1_tready),
      .space(), .occupied());

   axi_fifo #(.WIDTH(WIDTH), .SIZE(SIZE)) main_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata(int1_tdata), .i_tvalid(int1_tvalid), .i_tready(int1_tready),
      .o_tdata(int2_tdata), .o_tvalid(int2_tvalid), .o_tready(int2_tready),
      .space(space), .occupied(occupied));   // May change unexpectedly, but are always conservative
  
   axi_fifo_flop2 #(.WIDTH(WIDTH)) post_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata(int2_tdata), .i_tvalid(int2_tvalid), .i_tready(int2_tready),
      .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
      .space(), .occupied());

endmodule // axi_fifo_cascade

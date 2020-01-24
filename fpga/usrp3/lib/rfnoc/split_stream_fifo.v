//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module split_stream_fifo
  #(parameter WIDTH=16,
    parameter FIFO_SIZE=5,
    parameter ACTIVE_MASK=4'b1111)
   (input clk, input reset, input clear,
    input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH-1:0] o0_tdata, output o0_tlast, output o0_tvalid, input o0_tready,
    output [WIDTH-1:0] o1_tdata, output o1_tlast, output o1_tvalid, input o1_tready,
    output [WIDTH-1:0] o2_tdata, output o2_tlast, output o2_tvalid, input o2_tready,
    output [WIDTH-1:0] o3_tdata, output o3_tlast, output o3_tvalid, input o3_tready);

   wire [WIDTH-1:0]    o0_tdata_int, o1_tdata_int, o2_tdata_int, o3_tdata_int;
   wire 	       o0_tlast_int, o1_tlast_int, o2_tlast_int, o3_tlast_int;
   wire 	       o0_tvalid_int, o1_tvalid_int, o2_tvalid_int, o3_tvalid_int;
   wire 	       o0_tready_int, o1_tready_int, o2_tready_int, o3_tready_int;
   
   split_stream #(.WIDTH(WIDTH), .ACTIVE_MASK(ACTIVE_MASK)) split_stream
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata(i_tdata), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
      .o0_tdata(o0_tdata_int), .o0_tlast(o0_tlast_int), .o0_tvalid(o0_tvalid_int), .o0_tready(o0_tready_int),
      .o1_tdata(o1_tdata_int), .o1_tlast(o1_tlast_int), .o1_tvalid(o1_tvalid_int), .o1_tready(o1_tready_int),
      .o2_tdata(o2_tdata_int), .o2_tlast(o2_tlast_int), .o2_tvalid(o2_tvalid_int), .o2_tready(o2_tready_int),
      .o3_tdata(o3_tdata_int), .o3_tlast(o3_tlast_int), .o3_tvalid(o3_tvalid_int), .o3_tready(o3_tready_int));

   generate
      if(ACTIVE_MASK[0])
	axi_fifo #(.WIDTH(WIDTH+1), .SIZE(FIFO_SIZE)) axi_fifo0
	  (.clk(clk), .reset(reset), .clear(clear),
	   .i_tdata({o0_tlast_int, o0_tdata_int}), .i_tvalid(o0_tvalid_int), .i_tready(o0_tready_int),
	   .o_tdata({o0_tlast, o0_tdata}), .o_tvalid(o0_tvalid), .o_tready(o0_tready),
	   .occupied(), .space());
      if(ACTIVE_MASK[1])
	axi_fifo #(.WIDTH(WIDTH+1), .SIZE(FIFO_SIZE)) axi_fifo1
	  (.clk(clk), .reset(reset), .clear(clear),
	   .i_tdata({o1_tlast_int, o1_tdata_int}), .i_tvalid(o1_tvalid_int), .i_tready(o1_tready_int),
	   .o_tdata({o1_tlast, o1_tdata}), .o_tvalid(o1_tvalid), .o_tready(o1_tready),
	   .occupied(), .space());
      if(ACTIVE_MASK[2])
	axi_fifo #(.WIDTH(WIDTH+1), .SIZE(FIFO_SIZE)) axi_fifo2
	  (.clk(clk), .reset(reset), .clear(clear),
	   .i_tdata({o2_tlast_int, o2_tdata_int}), .i_tvalid(o2_tvalid_int), .i_tready(o2_tready_int),
	   .o_tdata({o2_tlast, o2_tdata}), .o_tvalid(o2_tvalid), .o_tready(o2_tready),
	   .occupied(), .space());
      if(ACTIVE_MASK[3])
	axi_fifo #(.WIDTH(WIDTH+1), .SIZE(FIFO_SIZE)) axi_fifo3
	  (.clk(clk), .reset(reset), .clear(clear),
	   .i_tdata({o3_tlast_int, o3_tdata_int}), .i_tvalid(o3_tvalid_int), .i_tready(o3_tready_int),
	   .o_tdata({o3_tlast, o3_tdata}), .o_tvalid(o3_tvalid), .o_tready(o3_tready),
	   .occupied(), .space());
   endgenerate
      
endmodule // split_stream_fifo


// Copyright 2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
// axi_demux -- takes 1 AXI stream, demuxes to up to 16 output streams
//  One bubble cycle between each packet

module axi_demux
  #(parameter WIDTH=64,
    parameter SIZE=4,
    parameter PRE_FIFO_SIZE=0,
    parameter POST_FIFO_SIZE=0)
   (input clk, input reset, input clear,
    output [WIDTH-1:0] header, input [$clog2(SIZE)-1:0] dest,
    input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [(WIDTH*SIZE)-1:0] o_tdata, output [SIZE-1:0] o_tlast, output [SIZE-1:0] o_tvalid, input [SIZE-1:0] o_tready);

   wire i_tlast_int, i_tready_int, i_tvalid_int;
   wire [WIDTH-1:0] i_tdata_int;
   generate
     if (PRE_FIFO_SIZE == 0) begin
       assign i_tlast_int = i_tlast;
       assign i_tdata_int = i_tdata;
       assign i_tvalid_int = i_tvalid;
       assign i_tready = i_tready_int;
     end else begin
       axi_fifo #(.WIDTH(WIDTH+1),.SIZE(PRE_FIFO_SIZE)) axi_fifo (
         .clk(clk), .reset(reset), .clear(clear),
         .i_tdata({i_tlast,i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready),
         .o_tdata({i_tlast_int,i_tdata_int}), .o_tvalid(i_tvalid_int), .o_tready(i_tready_int),
         .space(), .occupied());
     end
   endgenerate

   reg [SIZE-1:0] 	      st;

   assign header = i_tdata_int;

   always @(posedge clk)
     if(reset | clear)
       st <= {SIZE{1'b0}};
     else
       if(st == 0)
	 if(i_tvalid_int)
	   st[dest] <= 1'b1;
	 else
	   ;
       else
	 if(i_tready_int & i_tvalid_int & i_tlast_int)
	   st <= {SIZE{1'b0}};
   
   wire [SIZE-1:0]  o_tlast_int, o_tready_int, o_tvalid_int;
   wire [WIDTH-1:0] o_tdata_int[0:SIZE-1];
   genvar n;
   generate
     if (POST_FIFO_SIZE == 0) begin
       assign o_tdata = {SIZE{i_tdata_int}};
       assign o_tlast = {SIZE{i_tlast_int}};
       assign o_tvalid = {SIZE{i_tvalid_int}} & st;
       assign i_tready_int = |(o_tready & st);
     end else begin
       wire [SIZE-1:0] o_tready_fifo;
       assign i_tready_int = |(o_tready_fifo & st);
       for (n = 0; n < SIZE; n = n + 1) begin
         axi_fifo #(.WIDTH(WIDTH+1),.SIZE(POST_FIFO_SIZE)) axi_fifo (
           .clk(clk), .reset(reset), .clear(clear),
           .i_tdata({i_tlast_int,i_tdata_int}), .i_tvalid(i_tvalid_int & st[n]), .i_tready(o_tready_fifo[n]),
           .o_tdata({o_tlast[n],o_tdata[WIDTH*(n+1)-1:WIDTH*n]}), .o_tvalid(o_tvalid[n]), .o_tready(o_tready[n]),
           .space(), .occupied());
       end
     end
   endgenerate

endmodule // axi_demux

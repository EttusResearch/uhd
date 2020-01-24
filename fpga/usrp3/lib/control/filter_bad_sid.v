//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Discard silently packets which don't match this SID

module filter_bad_sid
  (
   input clk,
   input reset,
   input clear,
   //
   input [64:0] i_tdata,
   input i_tvalid,
   output i_tready,
   //
   output [64:0] o_tdata,
   output o_tvalid,
   input  o_tready,
   //
   output reg [15:0] count
   );

   reg [1:0] state;
   wire      good_sid;
   wire      qualify_i_tvalid;
   
   localparam IDLE = 0;
   localparam ACCEPT = 1;
   localparam DISCARD = 2;
   

   always @(posedge clk)
     if (reset | clear) begin
	state <= IDLE;
	count <= 0;
     end else
       case(state)
	 //
	 IDLE: begin
	    if  (i_tvalid && i_tready)
	      if (good_sid)
		state <= ACCEPT;
	      else begin
		 count <= count + 1;
		 state <= DISCARD;
	      end	    
	 end
	 //
	 ACCEPT: begin
	    if (i_tvalid && i_tready && i_tdata[64]) 
	      state <= IDLE;
	 end
	 //
	 DISCARD: begin
	    if (i_tvalid && i_tready && i_tdata[64]) 
	      state <= IDLE;
	 end
       endcase // case(state)
   
   assign good_sid = ((i_tdata[15:0] == 16'h00A0) || (i_tdata[15:0] == 16'h00B0));
   
   assign qualify_i_tvalid = (state == IDLE) ? good_sid : ((state == DISCARD) ? 1'b0 : 1'b1);

   //
   // Buffer output, break combinatorial timing paths
   //
   axi_fifo_short #(.WIDTH(65)) fifo_short
     (
      .clk(clk), .reset(reset), .clear(clear),
      .i_tdata(i_tdata), .i_tvalid(i_tvalid && qualify_i_tvalid), .i_tready(i_tready),
      .o_tdata(o_tdata), .o_tvalid(o_tvalid), .o_tready(o_tready),
      .space(), .occupied()
      );
   
endmodule // filter_bad_sid

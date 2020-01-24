//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// FIXME -- detect seqnum errors?

module chdr_deframer #(
   parameter WIDTH = 32 // Can be 32 or 64
)( input clk, input reset, input clear,
   input [63:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
   output [WIDTH-1:0] o_tdata, output [127:0] o_tuser, output o_tlast, output o_tvalid, input o_tready);
   
   localparam ST_HEAD = 2'd0;
   localparam ST_TIME = 2'd1;
   localparam ST_BODY = 2'd2;
   
   reg [1:0] 	 chdr_state;
   reg 		 odd_length;

   wire [127:0]  hdr_i_tuser, hdr_o_tuser;
   wire 	 hdr_i_tvalid, hdr_i_tready;
   wire 	 hdr_o_tvalid, hdr_o_tready;

   wire [63:0] 	 body_i_tdata, body_o_tdata;
   wire 	 body_i_tlast, body_o_tlast;
   wire 	 body_i_tvalid, body_o_tvalid;
   wire 	 body_i_tready, body_o_tready;

   wire 	 has_time = i_tdata[61];
   wire [15:0] 	 len = i_tdata[47:32];
   reg [63:0] 	 held_i_tdata;
   
   assign body_i_tdata = i_tdata;
   assign body_i_tlast = i_tlast;
   assign body_i_tvalid = (chdr_state == ST_BODY) ? i_tvalid : 1'b0;

   assign hdr_i_tuser = (chdr_state == ST_HEAD) ? { i_tdata, i_tdata } : { held_i_tdata, i_tdata }; // 2nd half ignored if no time
   assign hdr_i_tvalid = (chdr_state == ST_TIME) ? i_tvalid :
			 ((chdr_state == ST_HEAD) & ~has_time) ? i_tvalid :
			 1'b0;
   
   assign i_tready = (chdr_state == ST_BODY) ? body_i_tready : hdr_i_tready;

   // FIXME handle packets with no body
   always @(posedge clk)
     if(reset | clear)
       chdr_state <= ST_HEAD;
     else
       case(chdr_state)
	 ST_HEAD :
	   if(i_tvalid & hdr_i_tready)
	     if(has_time)
	       begin
		  chdr_state <= ST_TIME;
		  held_i_tdata <= i_tdata;
	       end
	     else
	       chdr_state <= ST_BODY;
	 ST_TIME :
	   if(i_tvalid & hdr_i_tready)
	     chdr_state <= ST_BODY;
	 ST_BODY :
	   if(i_tvalid & body_i_tready & i_tlast)
	     chdr_state <= ST_HEAD;
       endcase // case (chdr_state)
   
   axi_fifo #(.WIDTH(128), .SIZE(5)) hdr_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata(hdr_i_tuser), .i_tvalid(hdr_i_tvalid), .i_tready(hdr_i_tready),
      .o_tdata(hdr_o_tuser), .o_tvalid(hdr_o_tvalid), .o_tready(hdr_o_tready),
      .occupied(), .space());
   
   axi_fifo #(.WIDTH(65), .SIZE(5)) body_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata({body_i_tlast, body_i_tdata}), .i_tvalid(body_i_tvalid), .i_tready(body_i_tready),
      .o_tdata({body_o_tlast, body_o_tdata}), .o_tvalid(body_o_tvalid), .o_tready(body_o_tready),
      .occupied(), .space());

   assign o_tuser = hdr_o_tuser;
   assign o_tvalid = hdr_o_tvalid & body_o_tvalid;
   assign hdr_o_tready = o_tvalid & o_tready & o_tlast;

   generate if (WIDTH == 32) begin
     reg second_half;
     wire odd_len = hdr_o_tuser[98] ^ |hdr_o_tuser[97:96];

     always @(posedge clk)
       if(reset | clear)
         second_half <= 1'b0;
       else
         if(o_tvalid & o_tready)
           if(o_tlast)
             second_half <= 1'b0;
           else
             second_half <= ~second_half;
     
     assign o_tdata = second_half ? body_o_tdata[31:0] : body_o_tdata[63:32];
     assign o_tlast = body_o_tlast & (second_half | odd_len);
     assign body_o_tready = o_tvalid & o_tready & (o_tlast | second_half);
   end else if (WIDTH == 64) begin
     assign o_tdata = body_o_tdata;
     assign o_tlast = body_o_tlast;
     assign body_o_tready = o_tvalid & o_tready;
   end endgenerate
 
endmodule // chdr_deframer

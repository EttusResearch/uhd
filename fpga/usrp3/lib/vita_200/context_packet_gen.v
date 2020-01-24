//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module context_packet_gen
  (input clk, input reset, input clear,
   input trigger,
   input [11:0] seqnum,
   input [31:0] sid,
   input [63:0] body,
   input [63:0] vita_time,

   output done,
   output reg [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);
   
   reg [1:0] 	     cp_state;
   localparam CP_IDLE = 2'd0;
   localparam CP_HEAD = 2'd1;
   localparam CP_TIME = 2'd2;
   localparam CP_DATA = 2'd3;
   
   always @(posedge clk)
     if(reset|clear)
       cp_state <= CP_IDLE;
     else
       case(cp_state)
	 CP_IDLE :
	   if(trigger)
	     cp_state <= CP_HEAD;
	 CP_HEAD :
	   if(o_tready)
	     cp_state <= CP_TIME;
	 CP_TIME :
	   if(o_tready)
	     cp_state <= CP_DATA;
	 CP_DATA :
	   if(o_tready)
	     cp_state <= CP_IDLE;
       endcase // case (cp_state)

   assign o_tvalid = (cp_state != CP_IDLE);
   assign o_tlast = (cp_state == CP_DATA);

   always @*
     case(cp_state)
       CP_HEAD : o_tdata <= { 4'hA, seqnum, 16'd24, sid };
       CP_TIME : o_tdata <= vita_time;
       CP_DATA : o_tdata <= body;
       default : o_tdata <= body;
     endcase // case (cp_state)

   assign done = o_tlast & o_tvalid & o_tready;
   
endmodule // context_packet_gen

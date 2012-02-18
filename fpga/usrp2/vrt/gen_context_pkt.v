//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//



module gen_context_pkt
  #(parameter PROT_ENG_FLAGS=1,
    parameter DSP_NUMBER=0)
   (input clk, input reset, input clear,
    input trigger, output sent,
    input [31:0] streamid,
    input [63:0] vita_time,
    input [31:0] message,
    input [31:0] seqnum,
    output [35:0] data_o, output src_rdy_o, input dst_rdy_i);
   
   localparam CTXT_IDLE = 0;
   localparam CTXT_PROT_ENG = 1;
   localparam CTXT_HEADER = 2;
   localparam CTXT_STREAMID = 3;
   localparam CTXT_TICS = 4;
   localparam CTXT_TICS2 = 5;
   localparam CTXT_MESSAGE = 6;
   localparam CTXT_FLOWCTRL = 7;
   localparam CTXT_DONE = 8;

   reg [33:0] 	 data_int;
   wire 	 src_rdy_int, dst_rdy_int;
   reg [3:0] 	 seqno;
   reg [3:0] 	 ctxt_state;
   reg [63:0] 	 err_time;
   reg [31:0] 	 stored_message;
   
   always @(posedge clk)
     if(reset | clear)
       stored_message <= 0;
     else
       if(trigger)
	 stored_message <= message;
       else if(ctxt_state == CTXT_DONE)
	 stored_message <= 0;

   // Don't want to clear most of this to avoid getting stuck with a half packet in the pipe
   always @(posedge clk)
     if(reset)
       begin
	  ctxt_state <= CTXT_IDLE;
	  seqno <= 0;
       end
     else
       case(ctxt_state)
	 CTXT_IDLE :
	   if(trigger)
	     begin
		err_time <= vita_time;
		if(PROT_ENG_FLAGS)
		  ctxt_state <= CTXT_PROT_ENG;
		else
		  ctxt_state <= CTXT_HEADER;
	     end
	 
	 CTXT_DONE :
	   begin
	      ctxt_state <= CTXT_IDLE;
	      seqno <= seqno + 4'd1;
	   end
	 default :
	   if(dst_rdy_int)
	     ctxt_state <= ctxt_state + 1;
       endcase // case (ctxt_state)

   assign src_rdy_int = ~( (ctxt_state == CTXT_IDLE) | (ctxt_state == CTXT_DONE) );

   always @*
     case(ctxt_state)
       CTXT_PROT_ENG : data_int <= { 2'b01, 13'b0, DSP_NUMBER[0], 1'b1, 1'b1, 16'd24 }; // UDP port 1 or 3
       CTXT_HEADER : data_int <= { 1'b0, (PROT_ENG_FLAGS ? 1'b0 : 1'b1), 12'b010100000001, seqno, 16'd6 };
       CTXT_STREAMID : data_int <= { 2'b00, streamid };
       CTXT_TICS : data_int <= { 2'b00, err_time[63:32] };
       CTXT_TICS2 : data_int <= { 2'b00, err_time[31:0] };
       CTXT_MESSAGE : data_int <= { 2'b00, message };
       CTXT_FLOWCTRL : data_int <= { 2'b10, seqnum };
       default : data_int <= {2'b00, 32'b00};
     endcase // case (ctxt_state)

   fifo_short #(.WIDTH(34)) ctxt_fifo
     (.clk(clk), .reset(reset), .clear(0),
      .datain(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int),
      .dataout(data_o[33:0]), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i));
   assign data_o[35:34] = 2'b00;
   
endmodule // gen_context_pkt

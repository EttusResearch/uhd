//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module new_tx_deframer
  (input clk, input reset, input clear,
   input [63:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
   output [175:0] sample_tdata, output sample_tvalid, input sample_tready, output [31:0] debug);

   reg 		  odd, send_at, eob;
   reg [11:0] 	  seqnum;
   reg [31:0] 	  sid;
   reg [63:0] 	  send_time;
      
   wire [175:0]   fifo_tdata = { odd, send_at, eob, i_tlast, seqnum/*12*/, sid, send_time/*64*/, i_tdata/*64*/ };
   wire 	  fifo_tvalid, fifo_tready;

   reg [1:0] 	  td_state;
   localparam TD_HEAD = 0;
   localparam TD_TIME = 1;
   localparam TD_BODY = 2;
   localparam TD_DUMP = 3;

   always @(posedge clk)
     if(reset | clear)
       begin
	  td_state <= TD_HEAD;
	  odd <= 1'b0;
	  send_at <= 1'b0;
	  eob <= 1'b0;
	  seqnum <= 12'd0;
	  sid <= 32'd0;
	  send_time <= 64'h0;
       end // if (reset | clear)
     else
       case(td_state)
	 TD_HEAD :
	   if(i_tvalid)
	     begin
		if(~i_tlast)
		  if(i_tdata[63])
		    td_state <= TD_DUMP;
		  else if(i_tdata[61])
		    td_state <= TD_TIME;
		  else
		    td_state <= TD_BODY;
		odd <= i_tdata[34];
		send_at <= i_tdata[61];
		eob <= i_tdata[60];
		seqnum <= i_tdata[59:48];
		sid <= i_tdata[31:0];
		// FIXME record trailer, length, and SID here
	     end
	 TD_TIME :
	   if(i_tvalid)
	     begin
		send_time <= i_tdata;
		if(~i_tlast)
		  td_state <= TD_BODY;
		else
		  td_state <= TD_HEAD;
	     end
	 TD_BODY :
	   if(i_tvalid & fifo_tready)
	     if(i_tlast)
	       td_state <= TD_HEAD;
	 TD_DUMP :
	   if(i_tvalid)
	     if(i_tlast)
	       td_state <= TD_HEAD;
       endcase // case (td_state)

   assign fifo_tvalid = i_tvalid & (td_state == TD_BODY);
   assign i_tready = (td_state == TD_BODY) ? fifo_tready : 1'b1;
   
   axi_fifo_short #(.WIDTH(176)) ofifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata(fifo_tdata), .i_tvalid(fifo_tvalid), .i_tready(fifo_tready),
      .o_tdata(sample_tdata), .o_tvalid(sample_tvalid), .o_tready(sample_tready),
      .space(), .occupied());


   assign debug = {
		   sample_tvalid, // [8]
		   sample_tready, // [7]
		   i_tvalid,  // [6]
		   i_tready,  // [5]
		   td_state,  // [4:3]
		   odd,       // [2]
		   send_at,   // [1]
		   eob        // [0]
		   };
   
endmodule // new_tx_deframer

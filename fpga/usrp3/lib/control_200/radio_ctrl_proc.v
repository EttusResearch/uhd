//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// Radio Control Processor
//  Accepts compressed vita extension context packets of the following form:
//       { VITA Compressed Header, Stream ID }
//       { Optional 64 bit time }
//       { 16'h0, setting bus address [15:0], setting [31:0] }
//
//  If there is a timestamp, packet is held until that time comes.
//  Goes immediately if there is no timestamp or if time has passed.
//  Sends out setting to setting bus, and then generates a response packet
//  with the same sequence number, the src/dest swapped streamid, and the actual time
//  the setting was sent.
//
// Note -- if t0 is the requested time, the actual send time on the setting bus is t0 + 1 cycle.

module radio_ctrl_proc
  (input clk, input reset, input clear,
   
   input [63:0] ctrl_tdata, input ctrl_tlast, input ctrl_tvalid, output reg ctrl_tready,
   output reg [63:0] resp_tdata, output reg resp_tlast, output resp_tvalid, input resp_tready,
   
   input [63:0] vita_time,

   output set_stb, output [7:0] set_addr, output [31:0] set_data,
   input ready,
   
   input [63:0] readback,
   
   output [31:0] debug);

   localparam RC_HEAD   = 4'd0;
   localparam RC_TIME   = 4'd1;
   localparam RC_DATA   = 4'd2;
   localparam RC_DUMP   = 4'd3;
   localparam RC_RESP_HEAD = 4'd4;
   localparam RC_RESP_TIME = 4'd5;
   localparam RC_RESP_DATA = 4'd6;
   
   wire 	 IS_EC = ctrl_tdata[63];
   wire 	 HAS_TIME = ctrl_tdata[61];
   reg 		 HAS_TIME_reg;
   
   reg [3:0] 	 rc_state;
   reg [63:0] 	 cmd_time;

   wire 	 now, late, go;
   reg [11:0] 	 seqnum;
   reg [31:0] 	 sid;
   
   always @(posedge clk)
     if(reset)
       begin
	  rc_state <= RC_HEAD;
	  HAS_TIME_reg <= 1'b0;
	  sid <= 32'd0;
	  seqnum <= 12'd0;
       end
     else
	 case(rc_state)
	   RC_HEAD :
	     if(ctrl_tvalid)
	       begin
		  sid <= ctrl_tdata[31:0];
		  seqnum <= ctrl_tdata[59:48];
		  HAS_TIME_reg <= HAS_TIME;
		  if(IS_EC)
		    if(HAS_TIME)
		      rc_state <= RC_TIME;
		    else
		      rc_state <= RC_DATA;
		  else
		    if(~ctrl_tlast)
		      rc_state <= RC_DUMP;
	       end
	   
	   RC_TIME :
	     if(ctrl_tvalid)
	       if(ctrl_tlast)
		 rc_state <= RC_RESP_HEAD;
	       else if(go)
		 rc_state <= RC_DATA;
	   
	   RC_DATA :
	     if(ctrl_tvalid)
	       if(ready)
		 if(ctrl_tlast)
		   rc_state <= RC_RESP_HEAD;
		 else
		   rc_state <= RC_DUMP;

	   RC_DUMP :
	     if(ctrl_tvalid)
	       if(ctrl_tlast)
		 rc_state <= RC_RESP_HEAD;

	   RC_RESP_HEAD :
	     if(resp_tready)
	       rc_state <= RC_RESP_TIME;

	   RC_RESP_TIME :
	     if(resp_tready)
	       rc_state <= RC_RESP_DATA;

	   RC_RESP_DATA:
	     if(resp_tready)
	       rc_state <= RC_HEAD;
	   
	   default :
	     rc_state <= RC_HEAD;
	 endcase // case (rc_state)

   always @*
     case (rc_state)
       RC_HEAD : ctrl_tready <= 1'b1;
       RC_TIME : ctrl_tready <= ctrl_tlast | go;
       RC_DATA : ctrl_tready <= ready;
       RC_DUMP : ctrl_tready <= 1'b1;
       default : ctrl_tready <= 1'b0;
     endcase // case (rc_state)
      
   time_compare time_compare
     (.clk(clk), .reset(reset), .time_now(vita_time), .trigger_time(ctrl_tdata), .now(now), .early(), .late(late), .too_early());

   assign go = now | late;
   
   assign set_stb = (rc_state == RC_DATA) & ready & ctrl_tvalid;
   assign set_addr = ctrl_tdata[39:32];
   assign set_data = ctrl_tdata[31:0];

   always @(posedge clk)
      if (set_stb)
         cmd_time <= vita_time;

   always @*
     case (rc_state)
       RC_RESP_HEAD : { resp_tlast, resp_tdata } <= {1'b0, 4'hA, seqnum, 16'd24, sid[15:0], sid[31:16] };
       RC_RESP_TIME : { resp_tlast, resp_tdata } <= {1'b0, cmd_time};
       RC_RESP_DATA : { resp_tlast, resp_tdata } <= {1'b1, readback};
       default :      { resp_tlast, resp_tdata } <= 65'h0;
     endcase // case (rc_state)
   
   assign resp_tvalid = (rc_state == RC_RESP_HEAD) | (rc_state == RC_RESP_TIME) | (rc_state == RC_RESP_DATA);
   
endmodule // radio_ctrl_proc


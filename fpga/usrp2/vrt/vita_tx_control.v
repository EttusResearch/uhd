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


module vita_tx_control
  #(parameter BASE=0,
    parameter WIDTH=32)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    
    input [63:0] vita_time,
    output error, output ack,
    output reg [31:0] error_code,
    output reg packet_consumed,
    
    // From vita_tx_deframer
    input [5+64+16+WIDTH-1:0] sample_fifo_i,
    input sample_fifo_src_rdy_i,
    output sample_fifo_dst_rdy_o,
    
    // To DSP Core
    output [WIDTH-1:0] sample,
    output reg run,
    input strobe,

    output [31:0] debug
    );

   wire [63:0] send_time = sample_fifo_i[63:0];
   wire [15:0] seqnum = sample_fifo_i[79:64];
   wire        eop = sample_fifo_i[80];
   wire        eob = sample_fifo_i[81];
   wire        sob = sample_fifo_i[82];
   wire        send_at = sample_fifo_i[83];
   wire        seqnum_err = sample_fifo_i[84];
   
   wire        now, early, late, too_early;

   time_compare 
     time_compare (.time_now(vita_time), .trigger_time(send_time), 
		   .now(now), .early(early), .late(late), .too_early(too_early));

   reg 	       late_qual, late_del;

   always @(posedge clk)
     if(reset | clear)
       late_del <= 0;
     else
       late_del <= late;
   
   always @(posedge clk)
     if(reset | clear)
       late_qual <= 0;
     else
       late_qual <= (sample_fifo_src_rdy_i & ~sample_fifo_dst_rdy_o);
      
   localparam IBS_IDLE = 0;
   localparam IBS_RUN = 1;  // FIXME do we need this?
   localparam IBS_CONT_BURST = 2;
   localparam IBS_ERROR = 3;
   localparam IBS_ERROR_DONE = 4;
   localparam IBS_ERROR_WAIT = 5;

   wire [31:0] CODE_EOB_ACK = {seqnum,16'd1};
   wire [31:0] CODE_UNDERRUN = {seqnum,16'd2};
   wire [31:0] CODE_SEQ_ERROR = {seqnum,16'd4};
   wire [31:0] CODE_TIME_ERROR = {seqnum,16'd8};
   wire [31:0] CODE_UNDERRUN_MIDPKT = {seqnum,16'd16};
   wire [31:0] CODE_SEQ_ERROR_MIDBURST = {seqnum,16'd32};
   
   reg [2:0] ibs_state;

   wire [31:0] error_policy;
   setting_reg #(.my_addr(BASE+3)) sr_error_policy
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(error_policy),.changed());

   wire        policy_wait = error_policy[0];
   wire        policy_next_packet = error_policy[1];
   wire        policy_next_burst = error_policy[2];
   reg 	       send_error, send_ack;
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  ibs_state <= IBS_IDLE;
	  send_error <= 0;
	  send_ack <= 0;
	  error_code <= 0;
       end
     else
       case(ibs_state)
	 IBS_IDLE :
	   if(sample_fifo_src_rdy_i)
	     if(seqnum_err)
	       begin
		  ibs_state <= IBS_ERROR;
		  error_code <= CODE_SEQ_ERROR;
		  send_error <= 1;
	       end
	     else if(~send_at | now)
	       ibs_state <= IBS_RUN;
	     else if((late_qual & late_del) | too_early)
	       begin
		  ibs_state <= IBS_ERROR;
		  error_code <= CODE_TIME_ERROR;
		  send_error <= 1;
	       end
	 
	 IBS_RUN :
	   if(strobe)
	     if(~sample_fifo_src_rdy_i)
	       begin
		  ibs_state <= IBS_ERROR;
		  error_code <= CODE_UNDERRUN_MIDPKT;
		  send_error <= 1;
	       end
	     else if(eop)
	       if(eob)
		 begin
		    ibs_state <= IBS_ERROR_DONE;  // Not really an error
		    error_code <= CODE_EOB_ACK;
		    send_ack <= 1;
		 end
	       else
		 ibs_state <= IBS_CONT_BURST;

	 IBS_CONT_BURST :
	   if(strobe)
	     begin
		if(policy_next_packet)
		  ibs_state <= IBS_ERROR_DONE;
		else if(policy_wait)
		  ibs_state <= IBS_ERROR_WAIT;
		else
		  ibs_state <= IBS_ERROR;
		error_code <= CODE_UNDERRUN;
		send_error <= 1;
	     end
	   else if(sample_fifo_src_rdy_i)
	     if(seqnum_err)
	       begin
		  ibs_state <= IBS_ERROR;
		  error_code <= CODE_SEQ_ERROR_MIDBURST;
		  send_error <= 1;
	       end
	     else
	       ibs_state <= IBS_RUN;
	 
	 IBS_ERROR :
	   begin
	      send_error <= 0;
	      if(sample_fifo_src_rdy_i & eop)
		if(policy_next_packet | (policy_next_burst & eob))
		  ibs_state <= IBS_IDLE;
		else if(policy_wait)
		  ibs_state <= IBS_ERROR_WAIT;
	   end

	 IBS_ERROR_DONE :
	   begin
	      send_error <= 0;
	      send_ack <= 0;
	      ibs_state <= IBS_IDLE;
	   end
	 
	 IBS_ERROR_WAIT :
	   send_error <= 0;
       endcase // case (ibs_state)

   
   assign sample_fifo_dst_rdy_o = (ibs_state == IBS_ERROR) | (strobe & (ibs_state == IBS_RUN));  // FIXME also cleanout

   //register the output sample
   reg [31:0] sample_held;
   assign sample = sample_held;
   always @(posedge clk)
     if(reset | clear)
        sample_held <= 0;
     else if (~run)
       sample_held <= 0;
     else if (strobe)
       sample_held <= sample_fifo_i[5+64+16+WIDTH-1:5+64+16];

   assign error = send_error;
   assign ack = send_ack;

   localparam MAX_IDLE = 1000000; 
   // approx 10 ms timeout with a 100 MHz clock, but burning samples will slow that down
   reg [19:0] countdown;
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  run <= 0;
	  countdown <= 0;
       end
     else 
       if (ibs_state == IBS_RUN)
	 if(eob & eop & strobe & sample_fifo_src_rdy_i)
	   run <= 0;
	 else 
	   begin
	      run <= 1;
	      countdown <= MAX_IDLE;
	   end
       else
	 if (countdown == 0)
	   run <= 0;
	 else
	   countdown <= countdown - 1;
   	   
   always @(posedge clk)
     if(reset | clear)
       packet_consumed <= 0;
     else
       packet_consumed <= eop & sample_fifo_src_rdy_i & sample_fifo_dst_rdy_o;
   
   assign debug = { { now,late_qual,late_del,ack,eop,eob,sob,send_at },
		    { sample_fifo_src_rdy_i, sample_fifo_dst_rdy_o, strobe, run, error, ibs_state[2:0] },
		    { 8'b0 },
		    { 8'b0 } };
   
endmodule // vita_tx_control

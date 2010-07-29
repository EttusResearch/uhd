
module vita_tx_control
  #(parameter BASE=0,
    parameter WIDTH=32)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    
    input [63:0] vita_time,
    output error,
    output reg [31:0] error_code,

    // From vita_tx_deframer
    input [5+64+16+WIDTH-1:0] sample_fifo_i,
    input sample_fifo_src_rdy_i,
    output sample_fifo_dst_rdy_o,
    
    // To DSP Core
    output [WIDTH-1:0] sample,
    output run,
    input strobe,

    output [31:0] debug
    );

   assign sample = sample_fifo_i[5+64+16+WIDTH-1:5+64+16];

   wire [63:0] send_time = sample_fifo_i[63:0];
   wire [15:0] seqnum = sample_fifo_i[79:64];
   wire        eop = sample_fifo_i[80];
   wire        eob = sample_fifo_i[81];
   wire        sob = sample_fifo_i[82];
   wire        send_at = sample_fifo_i[83];
   wire        seqnum_err = sample_fifo_i[84];
   
   wire        now, early, late, too_early;

   // FIXME ignore too_early for now for timing reasons
   assign too_early = 0;
   time_compare 
     time_compare (.time_now(vita_time), .trigger_time(send_time), .now(now), .early(early), 
		   .late(late), .too_early());
//		   .late(late), .too_early(too_early));
   
   localparam IBS_IDLE = 0;
   localparam IBS_RUN = 1;  // FIXME do we need this?
   localparam IBS_CONT_BURST = 2;
   localparam IBS_ERROR = 3;
   localparam IBS_ERROR_DONE = 4;
   localparam IBS_ERROR_WAIT = 5;

   wire [31:0] CODE_UNDERRUN = {seqnum,16'd2};
   wire [31:0] CODE_SEQ_ERROR = {seqnum,16'd4};
   wire [31:0] CODE_TIME_ERROR = {seqnum,16'd8};
   wire [31:0] CODE_UNDERRUN_MIDPKT = {seqnum,16'd16};
   wire [31:0] CODE_SEQ_ERROR_MIDBURST = {seqnum,16'd32};
   
   reg [2:0] ibs_state;

   wire      clear_state;
   setting_reg #(.my_addr(BASE+1)) sr
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(),.changed(clear_state));
   
   wire [31:0] error_policy;
   setting_reg #(.my_addr(BASE+3)) sr_error_policy
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(error_policy),.changed());

   wire        policy_wait = error_policy[0];
   wire        policy_next_packet = error_policy[1];
   wire        policy_next_burst = error_policy[2];
   reg 	       send_error;
   
   always @(posedge clk)
     if(reset | clear_state)
       begin
	  ibs_state <= IBS_IDLE;
	  send_error <= 0;
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
	     else if(late | too_early)
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
		 ibs_state <= IBS_IDLE;
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
	   send_error <= 0;
	 
	 IBS_ERROR_WAIT :
	   send_error <= 0;
       endcase // case (ibs_state)

   assign sample_fifo_dst_rdy_o = (ibs_state == IBS_ERROR) | (strobe & (ibs_state == IBS_RUN));  // FIXME also cleanout
   assign run = (ibs_state == IBS_RUN) | (ibs_state == IBS_CONT_BURST);
   //assign error = (ibs_state == IBS_ERROR_DONE);
   assign error = send_error;

   assign debug = { { now,early,late,too_early,eop,eob,sob,send_at },
		    { sample_fifo_src_rdy_i, sample_fifo_dst_rdy_o, strobe, run, error, ibs_state[2:0] },
		    { 8'b0 },
		    { 8'b0 } };
   
endmodule // vita_tx_control

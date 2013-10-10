

module new_tx_control
  #(parameter BASE=0)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    
    input [63:0] vita_time,
    output reg ack_or_error,
    output packet_consumed,
    output [11:0] seqnum,
    output reg [63:0] error_code,
    output [31:0] sid,
    
    // From tx_deframer
    input [175:0] sample_tdata,
    input sample_tvalid,
    output sample_tready,
    
    // To DSP Core
    output [31:0] sample,    
    output run,    input strobe,
    
    output [31:0] debug
    );
   
   wire [31:0] 	  sample1 = sample_tdata[31:0];
   wire [31:0] 	  sample0 = sample_tdata[63:32];
   wire [63:0] 	  send_time = sample_tdata[127:64];
   assign         sid = sample_tdata[159:128];
   assign 	  seqnum = sample_tdata[171:160];
   wire 	  eop = sample_tdata[172];
   wire 	  eob = sample_tdata[173];
   wire 	  send_at = sample_tdata[174];
   wire 	  odd = sample_tdata[175];
   
   wire 	  now, early, late, too_early;
   wire 	  policy_next_burst, policy_next_packet, policy_wait;
   wire 	  clear_seqnum;
   
   setting_reg #(.my_addr(BASE), .width(3)) sr_error_policy
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out({policy_next_burst,policy_next_packet,policy_wait}),.changed(clear_seqnum));

   time_compare 
     time_compare (.clk(clk), .reset(reset), .time_now(vita_time), .trigger_time(send_time),
		   .now(now), .early(early), .late(late), .too_early(too_early));

   assign run = (state == ST_SAMP0) | (state == ST_SAMP1);
   
   assign sample = (state == ST_SAMP0) ? sample0 : sample1;
   
   reg [2:0] 	 state;
   
   localparam ST_IDLE  = 0;
   localparam ST_SAMP0 = 1;
   localparam ST_SAMP1 = 2;
   localparam ST_ERROR = 3;
   localparam ST_WAIT  = 4;

   wire [63:0] CODE_EOB_ACK            = {32'd1,20'd0,seqnum};
   wire [63:0] CODE_UNDERRUN           = {32'd2,20'd0,seqnum};
   wire [63:0] CODE_SEQ_ERROR          = {32'd4,20'd0,seqnum};
   wire [63:0] CODE_TIME_ERROR         = {32'd8,20'd0,seqnum};
   wire [63:0] CODE_UNDERRUN_MIDPKT    = {32'd16,20'd0,seqnum};
   wire [63:0] CODE_SEQ_ERROR_MIDBURST = {32'd32,20'd0,seqnum};

   reg [11:0]  expected_seqnum;

   always @(posedge clk)
     if(reset | clear | clear_seqnum)
       expected_seqnum <= 12'd0;
     else
       if(sample_tvalid & sample_tready & eop)
	 expected_seqnum <= seqnum + 12'd1;
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  state <= ST_IDLE;
	  ack_or_error <= 1'b0;
	  error_code <= 64'd0;
       end     
     else
       case(state)
	 ST_IDLE :
	   begin
	      ack_or_error <= 1'b0;
	      if(sample_tvalid)
		if(~send_at | now)
		  if(expected_seqnum != seqnum)
		    begin
		       state <= ST_ERROR;
		       ack_or_error <= 1'b1;
		       error_code <= CODE_SEQ_ERROR;
		    end
		  else
		    state <= ST_SAMP0;
		else if(late)
		  begin
		     state <= ST_ERROR;
		     ack_or_error <= 1'b1;
		     error_code <= CODE_TIME_ERROR;
		  end
	   end // case: ST_IDLE
	 ST_SAMP0 :
	   if(strobe)
	     if(~sample_tvalid)
	       begin
		  state <= ST_ERROR;
		  ack_or_error <= 1'b1;
		  error_code <= CODE_UNDERRUN;
	       end
	     else if(eop & odd & eob)
	       begin
		  state <= ST_IDLE;
		  ack_or_error <= 1'b1;
		  error_code <= CODE_EOB_ACK;
	       end
	     else if(eop & odd)
	       state <= ST_SAMP0;
	     else if(expected_seqnum != seqnum)
	       begin
		  state <= ST_ERROR;
		  ack_or_error <= 1'b1;
		  error_code <= CODE_SEQ_ERROR_MIDBURST;
	       end
	     else
	       state <= ST_SAMP1;
	 ST_SAMP1 :
	   if(strobe)
	     if(eop & eob)
	       begin
		  state <= ST_IDLE;
		  ack_or_error <= 1'b1;
		  error_code <= CODE_EOB_ACK;
	       end
	     else
	       state <= ST_SAMP0;
	 ST_ERROR :
	   begin
	      ack_or_error <= 1'b0;
	      if(sample_tvalid & eop)
		if(policy_next_packet | (policy_next_burst & eob))
		  state <= ST_IDLE;
		else if(policy_wait)
		  state <= ST_WAIT;
	   end
       endcase // case (state)

   assign sample_tready = (state == ST_ERROR) | (strobe & ( (state == ST_SAMP1) | ((state == ST_SAMP0) & eop & odd) ) );

   assign packet_consumed = eop & sample_tvalid & sample_tready;
   
   assign debug = {
		   error_code[15:0], // [28:13]
		   sample_tvalid, //[12]
		   now, // [11]
		   early, // [10]
		   late, // [9]
		   too_early, // [8]
		   strobe,  // [7]
		   eop, // [6]
		   eob, // [5]
		   send_at, // [4]
		   odd,  // [3]
		   state //  [2:0]
		   };

   
endmodule // new_tx_control

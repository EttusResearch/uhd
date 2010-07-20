
module vita_tx_control
  #(parameter BASE=0,
    parameter WIDTH=32)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    
    input [63:0] vita_time,
    output reg [3:0] error_code,
    output error,

    // From vita_tx_deframer
    input [5+64+WIDTH-1:0] sample_fifo_i,
    input sample_fifo_src_rdy_i,
    output sample_fifo_dst_rdy_o,
    
    // To DSP Core
    output [WIDTH-1:0] sample,
    output run,
    input strobe,

    output [31:0] debug
    );

   assign sample = sample_fifo_i[5+64+WIDTH-1:5+64];

   wire [63:0] send_time = sample_fifo_i[63:0];
   wire        eop = sample_fifo_i[64];
   wire        eob = sample_fifo_i[65];
   wire        sob = sample_fifo_i[66];
   wire        send_at = sample_fifo_i[67];
   wire        seqnum_err = sample_fifo_i[68];
   
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
   localparam IBS_UNDERRUN = 3;
   localparam IBS_TIME_ERROR = 4;
   localparam IBS_SEQ_ERROR = 5;
   localparam IBS_ERROR_DONE = 7;

   localparam CODE_UNDERRUN = 2;
   localparam CODE_SEQ_ERROR = 4;
   localparam CODE_TIME_ERROR = 8;
   
   reg [2:0] ibs_state;

   wire      clear_state;
   setting_reg #(.my_addr(BASE+1)) sr
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(),.changed(clear_state));
   
   always @(posedge clk)
     if(reset | clear_state)
       ibs_state <= 0;
     else
       case(ibs_state)
	 IBS_IDLE :
	   if(sample_fifo_src_rdy_i)
	     if(seqnum_err)
	       ibs_state <= IBS_SEQ_ERROR;
	     else if(~send_at | now)
	       ibs_state <= IBS_RUN;
	     else if(late | too_early)
	       ibs_state <= IBS_TIME_ERROR;
	 
	 IBS_RUN :
	   if(strobe)
	     if(~sample_fifo_src_rdy_i)
	       ibs_state <= IBS_UNDERRUN;
	     else if(eop)
	       if(eob)
		 ibs_state <= IBS_IDLE;
	       else
		 ibs_state <= IBS_CONT_BURST;

	 IBS_CONT_BURST :
	   if(strobe)
	     ibs_state <= IBS_ERROR_DONE;
	   else if(sample_fifo_src_rdy_i)
	     if(seqnum_err)
	       ibs_state <= IBS_SEQ_ERROR;
	     else
	       ibs_state <= IBS_RUN;
	 
	 IBS_UNDERRUN :
	   begin
	      error_code <= CODE_UNDERRUN;
	      if(sample_fifo_src_rdy_i & eop)
		ibs_state <= IBS_ERROR_DONE;
	   end
	 IBS_TIME_ERROR :
	   begin
	      error_code <= CODE_TIME_ERROR;
	      ibs_state <= IBS_ERROR_DONE;
	   end
	 IBS_SEQ_ERROR :
	   begin
	      error_code <= CODE_SEQ_ERROR;
	      ibs_state <= IBS_ERROR_DONE;
	   end
	 IBS_ERROR_DONE :
	   ;

       endcase // case (ibs_state)

   assign sample_fifo_dst_rdy_o = (ibs_state == IBS_UNDERRUN) | (strobe & (ibs_state == IBS_RUN));  // FIXME also cleanout
   assign run = (ibs_state == IBS_RUN) | (ibs_state == IBS_CONT_BURST);
   assign error = (ibs_state == IBS_ERROR_DONE);

   assign debug = { { now,early,late,too_early,eop,eob,sob,send_at },
		    { sample_fifo_src_rdy_i, sample_fifo_dst_rdy_o, strobe, run, error, ibs_state[2:0] },
		    { 8'b0 },
		    { 8'b0 } };
   
endmodule // vita_tx_control

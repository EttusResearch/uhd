

module gen_context_pkt
  #(parameter PROT_ENG_FLAGS=1)
   (input clk, input reset, input clear,
    input trigger, input error, output sent,
    input [31:0] streamid,
    input [63:0] vita_time,
    input [31:0] message,
    input [15:0] seqnum0,
    input [15:0] seqnum1,
    output [35:0] data_o, output src_rdy_o, input dst_rdy_i);
   
   localparam CTXT_IDLE = 0;
   localparam CTXT_PROT_ENG = 1;
   localparam CTXT_HEADER = 2;
   localparam CTXT_STREAMID = 3;
   localparam CTXT_SECS = 4;
   localparam CTXT_TICS = 5;
   localparam CTXT_TICS2 = 6;
   localparam CTXT_MESSAGE = 7;
   localparam CTXT_FLOWCTRL = 8;
   localparam CTXT_DONE = 9;

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
       if(error)
	 stored_message <= message;
       else if(ctxt_state == CTXT_FLOWCTRL)
	 stored_message <= 0;
   
   always @(posedge clk)
     if(reset | clear)
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
       CTXT_PROT_ENG : data_int <= { 2'b01, 16'd1, 16'd28 };
       CTXT_HEADER : data_int <= { 1'b0, (PROT_ENG_FLAGS ? 1'b0 : 1'b1), 12'b010100001101, seqno, 16'd7 };
       CTXT_STREAMID : data_int <= { 2'b00, streamid };
       CTXT_SECS : data_int <= { 2'b00, err_time[63:32] };
       CTXT_TICS : data_int <= { 2'b00, 32'd0 };
       CTXT_TICS2 : data_int <= { 2'b00, err_time[31:0] };
       CTXT_MESSAGE : data_int <= { 2'b00, message };
       CTXT_FLOWCTRL : data_int <= { 2'b10, {seqnum1,seqnum0} };
       default : data_int <= {2'b00, 32'b00};
     endcase // case (ctxt_state)

   fifo_short #(.WIDTH(34)) ctxt_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int),
      .dataout(data_o[33:0]), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i));
   assign data_o[35:34] = 2'b00;
   
endmodule // gen_context_pkt

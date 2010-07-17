

module gen_context_pkt
  #(parameter PROT_ENG_FLAGS=1)
   (input clk, input reset, input clear,
    input trigger, output sent,
    input [31:0] streamid,
    input [63:0] vita_time,
    input [31:0] message,
    output [35:0] data_o, output src_rdy_o, input dst_rdy_i);
   
   localparam CTXT_IDLE = 0;
   localparam CTXT_PROT_ENG = 1;
   localparam CTXT_HEADER = 2;
   localparam CTXT_STREAMID = 3;
   localparam CTXT_SECS = 4;
   localparam CTXT_TICS = 5;
   localparam CTXT_TICS2 = 6;
   localparam CTXT_MESSAGE = 7;
   localparam CTXT_DONE = 8;

   reg [33:0] 	 data_int;
   wire 	 src_rdy_int, dst_rdy_int;
   wire [3:0] 	 seqno = 0;
   reg [3:0] 	 ctxt_state;
   reg [63:0] 	 err_time;
   
   always @(posedge clk)
     if(reset | clear)
       ctxt_state <= CTXT_IDLE;
     else
       case(ctxt_state)
	 CTXT_IDLE :
	   if(trigger)
	     begin
		ctxt_state <= CTXT_HEADER;
		err_time <= vita_time;
	     end
	 
	 CTXT_DONE :
	   if(~trigger)
	     ctxt_state <= CTXT_IDLE;

	 default :
	   if(dst_rdy_int)
	     ctxt_state <= ctxt_state + 1;
       endcase // case (ctxt_state)

   assign src_rdy_int = ~( (ctxt_state == CTXT_IDLE) | (ctxt_state == CTXT_DONE) );
   
   always @*
     case(ctxt_state)
       CTXT_HEADER : data_int <= { 2'b01, 12'b010100001101, seqno, 16'd6 };
       CTXT_STREAMID : data_int <= { 2'b00, streamid };
       CTXT_SECS : data_int <= { 2'b00, err_time[63:32] };
       CTXT_TICS : data_int <= { 2'b00, 32'd0 };
       CTXT_TICS2 : data_int <= { 2'b00, err_time[31:0] };
       CTXT_MESSAGE : data_int <= { 2'b10, message };
       default : data_int <= {2'b00, 32'b00};
     endcase // case (ctxt_state)

   fifo_short #(.WIDTH(34)) ctxt_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int),
      .dataout(data_o[33:0]), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i));
   assign data_o[35:34] = 2'b00;
   
endmodule // gen_context_pkt

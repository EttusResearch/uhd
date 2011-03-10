
module fifo36_to_ll8
  (input clk, input reset, input clear,
   input [35:0] f36_data,
   input f36_src_rdy_i,
   output f36_dst_rdy_o,

   output [7:0] ll_data,
   output ll_sof,
   output ll_eof,
   output ll_src_rdy,
   input ll_dst_rdy,

   output [31:0] debug);

   // Shortfifo on input to guarantee no deadlock
   wire [35:0] 	 f36_data_int;
   wire 	 f36_src_rdy_int, f36_dst_rdy_int;
   reg [7:0] 	 ll_data_int;
   wire 	 ll_sof_int, ll_eof_int, ll_src_rdy_int, ll_dst_rdy_int;
   
   fifo_short #(.WIDTH(36)) head_fifo
     (.clk(clk),.reset(reset),.clear(clear),
      .datain(f36_data), .src_rdy_i(f36_src_rdy_i), .dst_rdy_o(f36_dst_rdy_o),
      .dataout(f36_data_int), .src_rdy_o(f36_src_rdy_int), .dst_rdy_i(f36_dst_rdy_int),
      .space(),.occupied() );

   // Actual fifo36 to ll8, can deadlock if not connected to shortfifo
   wire [1:0] 	 f36_occ_int = f36_data_int[35:34];
   wire 	 f36_sof_int = f36_data_int[32];
   wire 	 f36_eof_int = f36_data_int[33];
   wire 	 advance, end_early;
   reg [1:0] 	 state;
   
   assign debug    = {29'b0,state};

   always @(posedge clk)
     if(reset)
       state 	  <= 0;
     else
       if(advance)
	 if(ll_eof_int)
	   state  <= 0;
	 else
	   state  <= state + 1;

   always @*
     case(state)
       0 : ll_data_int = f36_data_int[31:24];
       1 : ll_data_int = f36_data_int[23:16];
       2 : ll_data_int = f36_data_int[15:8];
       3 : ll_data_int = f36_data_int[7:0];
       default : ll_data_int = f36_data_int[31:24];
       endcase // case (state)
   
   assign ll_sof_int 	 = (state==0) & f36_sof_int;
   assign ll_eof_int 	 = f36_eof_int & (((state==0)&(f36_occ_int==1)) |
					  ((state==1)&(f36_occ_int==2)) |
					  ((state==2)&(f36_occ_int==3)) |
					  (state==3));
   
   assign ll_src_rdy_int = f36_src_rdy_int;
   
   assign advance 	 = ll_src_rdy_int & ll_dst_rdy_int;
   assign f36_dst_rdy_int= advance & ((state==3)|ll_eof_int);

   // Short FIFO on output to guarantee no deadlock
   ll8_shortfifo tail_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(ll_data_int), .sof_i(ll_sof_int), .eof_i(ll_eof_int),
      .error_i(0), .src_rdy_i(ll_src_rdy_int), .dst_rdy_o(ll_dst_rdy_int),
      .dataout(ll_data), .sof_o(ll_sof), .eof_o(ll_eof),
      .error_o(), .src_rdy_o(ll_src_rdy), .dst_rdy_i(ll_dst_rdy));

endmodule // fifo36_to_ll8

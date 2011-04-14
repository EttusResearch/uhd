

module vita_pkt_gen
  (input clk, input reset, input clear,
   input [15:0] len,
   output [35:0] data_o, output src_rdy_o, input dst_rdy_i);

   reg [15:0] 	     state;
   reg [31:0] 	     seq, data;
   
   wire 	     sof = (state == 0);
   wire 	     eof = (state == (len-1));
   wire 	     consume = src_rdy_o & dst_rdy_i;
   
   assign src_rdy_o = 1;

   always @(posedge clk)
     if(reset | clear)
       begin
	  state <= 0;
	  seq <= 0;
       end
     else
       if(consume)
	 if(eof)
	   begin
	      state <= 0;
	      seq <= seq + 1;
	   end
	 else
	   state <= state + 1;

   always @*
     case(state)
       0 :   data <= {24'h000,seq[3:0],len};
       1 :   data <= seq;
       default : data <= {~state,state};
     endcase // case (state)

   assign data_o = {2'b00, eof, sof, data};
   
endmodule // vita_pkt_gen

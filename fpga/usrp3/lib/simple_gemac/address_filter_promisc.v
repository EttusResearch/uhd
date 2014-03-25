//
// Copyright 2011 Ettus Research LLC
//




module address_filter_promisc
  (input clk,
   input reset,
   input go,
   input [7:0] data,
   output match,
   output done);

   reg [2:0] af_state;

   always @(posedge clk)
     if(reset)
       af_state     <= 0;
     else
       if(go)
	 af_state <= 1;//(data[0] == 1'b0) ? 1 : 7;
       else
	 case(af_state)
	   1 : af_state <= 2;
	   2 : af_state <= 3;
	   3 : af_state <= 4;
	   4 : af_state <= 5;
	   5 : af_state <= 6;
	   6, 7 : af_state <= 0;
	 endcase // case (af_state)

   assign match  = (af_state==6);
   assign done 	 = (af_state==6)|(af_state==7);
   
endmodule // address_filter_promisc

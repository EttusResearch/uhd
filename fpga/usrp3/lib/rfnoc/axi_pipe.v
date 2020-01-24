
// Copyright 2014 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later


module axi_pipe
  #(parameter STAGES=3)
   (input clk, input reset, input clear,
    input i_tlast, input i_tvalid, output i_tready,
    output o_tlast, output o_tvalid, input o_tready,
    output [STAGES-1:0] enables,
    output reg [STAGES-1:0] valids);

   assign o_tvalid = valids[STAGES-1];
   assign i_tready = enables[0];

   // //////////////////////////////
   // Valids
   genvar 		    i;
   generate
      for(i=1; i<STAGES; i=i+1)
	always @(posedge clk)
	  if(reset | clear)
	    valids[i] <= 1'b0;
      	  else
	    valids[i] <= valids[i-1] | (valids[i] & ~enables[i]);
   endgenerate

   always @(posedge clk)
     if(reset | clear)
       valids[0] <= 1'b0;
     else
       valids[0] <= i_tvalid | (valids[0] & ~enables[0]);

   // //////////////////////////////
   // Enables
   genvar 		    j;
   generate
      for(j=0; j<STAGES; j=j+1)
	assign enables[j] = o_tready | (|(~valids[STAGES-1:j]));
   endgenerate

   // /////////////////////////////
   // tlast
   reg [STAGES-1:0] 	    tlast;
   
   genvar 		    k;
   generate
      for(k=1; k<STAGES; k=k+1)
	always @(posedge clk)
	  if(reset | clear)
	    tlast[k] <= 1'b0;
	  else if(enables[k])
	    tlast[k] <= tlast[k-1];
   endgenerate

   always @(posedge clk)
	  if(reset | clear)
	    tlast[0] <= 1'b0;
	  else if(enables[0])
	    tlast[0] <= i_tlast;

   assign o_tlast = tlast[STAGES-1];
   
endmodule // axi_pipe

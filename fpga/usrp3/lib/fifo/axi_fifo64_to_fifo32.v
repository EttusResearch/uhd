//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module axi_fifo64_to_fifo32
  (input clk, input reset, input clear,
   input [63:0] i_tdata, input [2:0] i_tuser, input i_tlast, input i_tvalid, output i_tready,
   output [31:0] o_tdata, output [1:0] o_tuser, output o_tlast, output o_tvalid, input o_tready
   );

   wire 	 short_last = i_tlast & ((i_tuser == 3'd1) | (i_tuser == 3'd2) | (i_tuser == 3'd3) | (i_tuser == 3'd4));
   
   reg 		 state;
   always @(posedge clk)
     if(reset | clear)
       state <= 1'b0;
     else
       if(i_tvalid & o_tready)
	 case(state)
	   1'b0 :
	     if(~short_last)
	       state <= 1'b1;
	   1'b1 :
	     state <= 1'b0;
	 endcase // case (state)
   
   assign o_tdata = (state == 0) ? i_tdata[63:32] : i_tdata[31:0];
   assign o_tuser = o_tlast ? i_tuser[1:0] : 2'd0;
   assign o_tlast = i_tlast & ((state == 1'b1) | short_last);

   assign o_tvalid = i_tvalid;
   assign i_tready = o_tready & ((state == 1'b1) | short_last);
   		    
endmodule // axi_fifo64_to_fifo32

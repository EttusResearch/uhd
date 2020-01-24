//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// This delay doesn't use a fifo, and solves pipeline bubble issues.  
// FIXME -- issues are that it will generate output without input, and you can't reduce delay, only increase

module delay_type2
  #(parameter MAX_LEN_LOG2=10,
    parameter WIDTH=16,
    parameter DELAY_VAL=0)
   (input clk, input reset, input clear,
    input [MAX_LEN_LOG2-1:0] len,
    input [WIDTH-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   reg [MAX_LEN_LOG2-1:0] delay_count;

   wire 		  delay_done = delay_count >= len;
   
   always @(posedge clk)
     if(reset)
       delay_count <= 0;
     else
       if(~delay_done & o_tvalid & o_tready)
	 delay_count <= delay_count + 1;

   assign o_tdata = delay_done ? i_tdata : DELAY_VAL;
   assign o_tlast = delay_done ? i_tlast : 1'b0;      // FIXME think about this more, no answer is perfect in all situations
   assign o_tvalid = delay_done ? i_tvalid : 1'b1;
   assign i_tready = delay_done ? o_tready : 1'b0;
   
endmodule // delay_type2

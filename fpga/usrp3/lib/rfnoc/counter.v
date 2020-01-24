//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Simple counter, reset by i_tlast on input side.  i_tdata not connected
//  Most useful for indexing a RAM, creating a ramp, etc.

module counter
  #(parameter WIDTH=16)
   (input clk, input reset, input clear,
    input [WIDTH:0] max,
    input i_tlast, input i_tvalid, output i_tready,
    output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   reg [WIDTH-1:0]     count;
   
   wire 	       do_it = o_tready & i_tvalid;
   wire 	       done = (count >= (max-1));
   
   always @(posedge clk)
     if(reset | clear)
       count <= 0;
     else
       if(do_it)
	 if( done | i_tlast )
	   count <= 0;
   	 else
	   count <= count + 1;
   
   assign o_tdata = count;
   assign o_tlast = done | i_tlast;
   assign o_tvalid = i_tvalid;
   assign i_tready = do_it;
   
endmodule // counter

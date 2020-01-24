//
// Copyright 2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//



module srl
  #(parameter WIDTH=18)
    (input clk,
     input rst,
     input write,
     input [WIDTH-1:0] in,
     input [3:0] addr,
     output [WIDTH-1:0] out);
   
   genvar 		i;
   generate
      for (i=0;i<WIDTH;i=i+1)
	begin : gen_srl
	   SRL16E
	     srl16e(.Q(out[i]),
		    .A0(addr[0]),.A1(addr[1]),.A2(addr[2]),.A3(addr[3]),
		    .CE(write|rst),.CLK(clk),.D(in[i]));
	end
   endgenerate

endmodule // srl

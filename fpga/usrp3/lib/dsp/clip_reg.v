// -*- verilog -*-
//
//  USRP - Universal Software Radio Peripheral
//
//  Copyright (C) 2008 Matt Ettus
// 
//  SPDX-License-Identifier: LGPL-3.0-or-later
//

//

// Clipping "macro", keeps the bottom bits

module clip_reg
  #(parameter bits_in=0,
    parameter bits_out=0,
    parameter STROBED=1'b0)
    (input clk,
     input reset,
     input [bits_in-1:0] in,
     output reg [bits_out-1:0] out,
     input strobe_in,
     output reg strobe_out);
   
   wire [bits_out-1:0] temp;

   clip #(.bits_in(bits_in),.bits_out(bits_out)) clip (.in(in),.out(temp));

   always @(posedge clk) strobe_out <= reset ? 1'b0 : strobe_in;
   
   always @(posedge clk)
     if(strobe_in | ~STROBED)
       out <= temp;
   
endmodule // clip_reg


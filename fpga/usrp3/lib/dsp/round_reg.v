// -*- verilog -*-
//
//  USRP - Universal Software Radio Peripheral
//
//  Copyright (C) 2008 Matt Ettus
//

//

// Rounding "macro"
// Keeps the topmost bits, does proper 2s comp rounding (round-to-zero)

module round_reg
  #(parameter bits_in=0,
    parameter bits_out=0)
    (input clk,
     input [bits_in-1:0] in,
     output reg [bits_out-1:0] out,
     output reg [bits_in-bits_out:0] err);

   wire [bits_out-1:0] temp;
   wire [bits_in-bits_out:0] err_temp;
   
   round #(.bits_in(bits_in),.bits_out(bits_out)) round (.in(in),.out(temp), .err(err_temp));
   
   always @(posedge clk)
     out <= temp;

   always @(posedge clk)
     err <= err_temp;
   
endmodule // round_reg

//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//



module rx_dcoffset 
  #(parameter WIDTH=16,
    parameter ADDR=8'd0,
    parameter alpha_shift=20)
   (input clk, input rst, 
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [WIDTH-1:0] in, output [WIDTH-1:0] out);
   
   wire 	      set_now = set_stb & (ADDR == set_addr);
   
   reg 		      fixed;  // uses fixed offset
   wire [WIDTH-1:0]   fixed_dco;

   localparam int_width = WIDTH + alpha_shift;
   reg [int_width-1:0] integrator;
   wire [WIDTH-1:0]    quantized;

   always @(posedge clk)
     if(rst)
       begin
	  fixed <= 0;
	  integrator <= {int_width{1'b0}};
       end
     else if(set_now)
       begin
	  fixed <= set_data[31];
	  if(set_data[30])
	    integrator <= {set_data[29:0],{(int_width-30){1'b0}}};
       end
     else if(~fixed)
       integrator <= integrator +  {{(alpha_shift){out[WIDTH-1]}},out};

   round_sd #(.WIDTH_IN(int_width),.WIDTH_OUT(WIDTH)) round_sd
     (.clk(clk), .reset(rst), .in(integrator), .strobe_in(1'b1), .out(quantized), .strobe_out());
   
   add2_and_clip_reg #(.WIDTH(WIDTH)) add2_and_clip_reg
     (.clk(clk), .rst(rst), .in1(in), .in2(-quantized), .strobe_in(1'b1), .sum(out), .strobe_out());

endmodule // rx_dcoffset

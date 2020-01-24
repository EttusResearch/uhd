//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module rx_dcoffset 
  #(parameter WIDTH=16,
    parameter ADDR=8'd0,
    parameter alpha_shift=20)
   (input clk, input rst,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input in_stb, input [WIDTH-1:0] in,
    output out_stb, output [WIDTH-1:0] out);

   wire 	      set_now = set_stb & (ADDR == set_addr);
   reg 		      fixed;  // uses fixed offset
   wire [WIDTH-1:0]   fixed_dco;

   localparam int_width = WIDTH + alpha_shift;
   reg [int_width-1:0] integrator;
   reg                 integ_in_stb;
   wire [WIDTH-1:0]    quantized;

   always @(posedge clk) begin
     if(rst)
       begin
       integ_in_stb <= 0;
	  fixed <= 0;
	  integrator <= {int_width{1'b0}};
       end
     else if(set_now)
       begin
	  fixed <= set_data[31];
	  if(set_data[30])
	    integrator <= {set_data[29:0],{(int_width-30){1'b0}}};
       end
     else if(~fixed & in_stb)
       integrator <= integrator +  {{(alpha_shift){out[WIDTH-1]}},out};
     integ_in_stb <= in_stb;
   end

   round_sd #(.WIDTH_IN(int_width),.WIDTH_OUT(WIDTH)) round_sd
     (.clk(clk), .reset(rst), .in(integrator), .strobe_in(integ_in_stb), .out(quantized), .strobe_out());

   add2_and_clip_reg #(.WIDTH(WIDTH)) add2_and_clip_reg
     (.clk(clk), .rst(rst), .in1(in), .in2(-quantized), .strobe_in(in_stb), .sum(out), .strobe_out(out_stb));

endmodule // rx_dcoffset

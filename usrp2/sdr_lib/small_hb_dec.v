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

// Short halfband decimator (intended to be followed by another stage)
// Implements impulse responses of the form [A 0 B 0.5 B 0 A]
//
// These taps designed by halfgen4 from ldoolittle:
//   2 * 131072 * halfgen4(.75/8,2)
module small_hb_dec
  #(parameter WIDTH=18)
    (input clk,
     input rst,
     input bypass,
     input run,
     input stb_in,
     input [WIDTH-1:0] data_in,
     output reg stb_out,
     output [WIDTH-1:0] data_out);

   reg 			stb_in_d1;
   reg [WIDTH-1:0] 	data_in_d1;
   always @(posedge clk) stb_in_d1 <= stb_in;
   always @(posedge clk) data_in_d1 <= data_in;
   
   wire 		go;
   reg 			phase, go_d1, go_d2, go_d3, go_d4;
   always @(posedge clk)
     if(rst | ~run)
       phase <= 0;
     else if(stb_in_d1)
       phase <= ~phase;
   assign 		go = stb_in_d1 & phase;
   always @(posedge clk) 
     if(rst | ~run)
       begin
	  go_d1 <= 0;
	  go_d2 <= 0;
	  go_d3 <= 0;
	  go_d4 <= 0;
       end
     else
       begin
	  go_d1 <= go;
	  go_d2 <= go_d1;
	  go_d3 <= go_d2;
	  go_d4 <= go_d3;
       end

   wire [17:0] 		coeff_a = -10690;
   wire [17:0] 		coeff_b = 75809;
   
   reg [WIDTH-1:0] 	d1, d2, d3, d4 , d5, d6;
   always @(posedge clk)
     if(stb_in_d1 | rst)
       begin
	  d1 <= data_in_d1;
	  d2 <= d1;
	  d3 <= d2;
	  d4 <= d3;
	  d5 <= d4;
	  d6 <= d5;
       end

   reg [17:0] sum_a, sum_b, middle, middle_d1;
   wire [17:0] sum_a_unreg, sum_b_unreg;
   add2 #(.WIDTH(18)) add2_a (.in1(data_in_d1),.in2(d6),.sum(sum_a_unreg));
   add2 #(.WIDTH(18)) add2_b (.in1(d2),.in2(d4),.sum(sum_b_unreg));
   
   always @(posedge clk)
     if(go)
       begin
	  sum_a <= sum_a_unreg;
	  sum_b <= sum_b_unreg;
	  middle <= d3;
       end

   always @(posedge clk)
     if(go_d1)
       middle_d1 <= middle;
   
   wire [17:0] sum = go_d1 ? sum_b : sum_a;
   wire [17:0] coeff = go_d1 ? coeff_b : coeff_a;
   wire [35:0] 	 prod;   
   MULT18X18S mult(.C(clk), .CE(go_d1 | go_d2), .R(rst), .P(prod), .A(coeff), .B(sum) );
   
   reg [35:0] 	 accum;
   always @(posedge clk)
     if(rst)
       accum <= 0;
     else if(go_d2)
       accum <= {middle_d1[17],middle_d1[17],middle_d1,16'd0} + {prod};
     else if(go_d3)
       accum <= accum + {prod};
   
   wire [17:0] 	 accum_rnd;
   wire 	 stb_round;
   
   round_sd #(.WIDTH_IN(36),.WIDTH_OUT(18)) round_acc 
     (.clk(clk), .reset(rst), .in(accum), .strobe_in(go_d4), .out(accum_rnd), .strobe_out(stb_round));

   reg [17:0] 	 final_sum;
   always @(posedge clk)
     if(bypass)
       final_sum <= data_in_d1;
     else if(stb_round)
       final_sum <= accum_rnd;

   assign 	 data_out = final_sum;

   always @(posedge clk)
     if(rst)
       stb_out <= 0;
     else if(bypass)
       stb_out <= stb_in_d1;
     else
       stb_out <= stb_round;
endmodule // small_hb_dec

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



module capture_ddrlvds
  #(parameter WIDTH=7)
   (input clk,
    input ssclk_p,
    input ssclk_n,
    input [WIDTH-1:0] in_p,
    input [WIDTH-1:0] in_n,
    output reg [(2*WIDTH)-1:0] out);

   wire [WIDTH-1:0] 	   ddr_dat;
   wire 		   ssclk;
   wire [(2*WIDTH)-1:0]    out_pre1;
   reg [(2*WIDTH)-1:0] 	   out_pre2;
   
   IBUFGDS #(.IOSTANDARD("LVDS_33"), .DIFF_TERM("TRUE")) 
   clkbuf (.O(ssclk), .I(ssclk_p), .IB(ssclk_n));
   
   genvar 	       i;
   generate
      for(i = 0; i < WIDTH; i = i + 1)
	begin : gen_lvds_pins
	   IBUFDS #(.IOSTANDARD("LVDS_33"),.DIFF_TERM("FALSE")) ibufds 
	      (.O(ddr_dat[i]), .I(in_p[i]), .IB(in_n[i]) );
	   IDDR2 #(.DDR_ALIGNMENT("C1")) iddr2
	     (.Q0(out_pre1[2*i]), .Q1(out_pre1[(2*i)+1]), .C0(ssclk), .C1(~ssclk),
	      .CE(1'b1), .D(ddr_dat[i]), .R(1'b0), .S(1'b0));
	end
   endgenerate

   always @(posedge clk)
     out_pre2 <= out_pre1;

   always @(posedge clk)
     out      <= out_pre2;
   
endmodule // capture_ddrlvds

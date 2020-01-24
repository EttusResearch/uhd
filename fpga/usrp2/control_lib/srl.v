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


module srl
  #(parameter WIDTH=18)
    (input clk,
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
		    .CE(write),.CLK(clk),.D(in[i]));
	end
   endgenerate

endmodule // srl

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



module delay_line
  #(parameter WIDTH=32)
   (input clk,
    input [3:0] delay,
    input [WIDTH-1:0] din,
    output [WIDTH-1:0] dout);
    
   genvar 	       i;
   generate
      for (i=0;i<WIDTH;i=i+1)
	begin : gen_delay
	   SRL16E
	     srl16e(.Q(dout[i]),
		    .A0(delay[0]),.A1(delay[1]),.A2(delay[2]),.A3(delay[3]),
		    .CE(1),.CLK(clk),.D(din[i]));
	end
   endgenerate

endmodule // delay_line

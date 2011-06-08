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

module integrate
  #(parameter INPUTW = 16,
    parameter ACCUMW = 32,
    parameter OUTPUTW = 16)

   (input clk_i,
    input rst_i,
    input ena_i,

    input dump_i,
    input [INPUTW-1:0] data_i,

    output reg stb_o,
    output reg [OUTPUTW-1:0] integ_o
   );
   
   wire [ACCUMW-1:0] data_ext = {{ACCUMW-INPUTW{data_i[INPUTW-1]}},data_i};
   reg  [ACCUMW-1:0] accum;

   always @(posedge clk_i)
     if (rst_i | ~ena_i)
       begin
	  accum <= 0;
	  integ_o <= 0;
       end
     else
       if (dump_i)
	 begin
	    integ_o <= accum[ACCUMW-1:ACCUMW-OUTPUTW];
	    accum <= data_ext;
	 end
       else
	 accum <= accum + data_ext;

   always @(posedge clk_i)
     stb_o <= dump_i;
   
endmodule // integrate

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

module pipestage
  #(parameter TAGWIDTH = 1)
   (input clk,
    input reset,
    input stb_in,
    input stb_out,
    output reg valid,
    input [TAGWIDTH-1:0] tag_in,
    output reg [TAGWIDTH-1:0] tag_out);

   always @(posedge clk)
     if(reset)
       begin
	  valid <= 0;
	  tag_out <= 0;
       end
     else if(stb_in)
       begin
	  valid <= 1;
	  tag_out <= tag_in;
       end
     else if(stb_out)
       begin
	  valid <= 0;
	  tag_out <= 0;
       end
   
endmodule // pipestage

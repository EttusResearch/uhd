//
// Copyright 2011-2012 Ettus Research LLC
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

//----------------------------------------------------------------------
//-- A settings register is a peripheral for the settings register bus.
//-- When the settings register sees strobe abd a matching address,
//-- the outputs will be become registered to the given input bus.
//----------------------------------------------------------------------

module setting_reg
  #(parameter my_addr = 0, 
    parameter awidth = 8,
    parameter width = 32,
    parameter at_reset=0)
    (input clk, input rst, input strobe, input wire [awidth-1:0] addr,
     input wire [31:0] in, output reg [width-1:0] out, output reg changed);
   
   always @(posedge clk)
     if(rst)
       begin
	  out <= at_reset;
	  changed <= 1'b0;
       end
     else
       if(strobe & (my_addr==addr))
	 begin
	    out <= in[width-1:0];
	    changed <= 1'b1;
	 end
       else
	 changed <= 1'b0;
   
endmodule // setting_reg
